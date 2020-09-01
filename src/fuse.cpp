#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <thread>
#include <signal.h>
#include <sys/syscall.h>

#include <Rcpp.h>
#include <map>
#include <shared_mutex>
#include "utils.h"

#define BUFFER_SIZE (1024 * 1024)

using std::map;
using std::string;
typedef std::pair<const string, void *> value_type;

//A struct to store all fuse operations
static struct fuse_operations operations;
//A map to the altrep objects
static size_t counter = 0;
static map<const string, void *> altrep_map;
//A shared mutex that is used for accessing map
static std::shared_mutex mutex;

//process id and thread that the fuse will be running on
pid_t tid;
std::thread *thread;

// mount point
string mount_point;




#define HAS_KEY(path) (altrep_map.find(path) != altrep_map.end())
#define GET_VALUE(path) ((SEXP)altrep_map.at(path))

static int do_getattr(const char *path, struct stat *st)
{
    debug_print("[getattr] Called\n");
    debug_print("\tAttributes of %s requested\n", path);
    string name(path + 1L);
    // GNU's definitions of the attributes (http://www.gnu.org/software/libc/manual/html_node/Attribute-Meanings.html)
    st->st_uid = getuid();     // The owner of the file/directory is the user who mounted the filesystem
    st->st_gid = getgid();     // The group of the file/directory is the same as the group of the user who mounted the filesystem
    st->st_atime = time(NULL); // The last "a"ccess of the file/directory is right now
    st->st_mtime = time(NULL); // The last "m"odification of the file/directory is right now
    mutex.lock_shared();
    if (name != "" && !HAS_KEY(name))
    {
        mutex.unlock_shared();
        return -1;
    }

    if (name == "")
    {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    }
    else
    {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = get_object_size(GET_VALUE(name));
    }
    mutex.unlock_shared();
    return 0;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    debug_print("--> Getting The List of Files of %s\n", path);

    filler(buffer, ".", NULL, 0);  // Current Directory
    filler(buffer, "..", NULL, 0); // Parent Directory

    if (strcmp(path, "/") == 0) // If the user is trying to show the files/directories of the root directory show the following
    {
        mutex.lock_shared();
        for (auto i : altrep_map)
        {
            filler(buffer, i.first.c_str(), NULL, 0);
        }
        mutex.unlock_shared();
    }
    return 0;
}
// Fill the data that cannot be completely represent by an element of x
// e.g. x is an integer vector, offset = 1, size = 2. only the second and third bytes of x[1] is required
#define fill_mismatched_data(x, i, intra_elt_offset, copy_size, buffer, buffer_size) \
    switch (TYPEOF(x))                                                               \
    {                                                                                \
    case INTSXP:                                                                     \
        int elt = INTEGER_ELT(x, i);                                                 \
        memcpy(buffer, ((char *)&elt) + intra_elt_offset, copy_size);                \
        break;                                                                       \
    case LGLSXP:                                                                     \
        int elt = LOGICAL_ELT(x, i);                                                 \
        memcpy(buffer, ((char *)&elt) + intra_elt_offset, copy_size);                \
        break;                                                                       \
    case REALSXP:                                                                    \
        double elt = REAL_ELT(x, i);                                                 \
        memcpy(buffer, ((char *)&elt) + intra_elt_offset, copy_size);                \
        break;                                                                       \
    }

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
    if (size == 0)
    {
        return 0;
    }
    size_t read_size = size;
    string name(path + 1L);
    SEXP x;
    mutex.lock_shared();
    bool key_exist = HAS_KEY(name);
    if (key_exist)
    {
        x = GET_VALUE(name);
    }
    mutex.unlock_shared();
    if (!key_exist)
    {
        return -1;
    }
    
    size_t elt_size = get_type_size(TYPEOF(x));
    size_t start_offset = offset / elt_size;
    size_t start_intra_elt_offset = offset - start_offset * elt_size;
    if (start_intra_elt_offset != 0)
    {
        size_t copy_size = std::max(elt_size - start_intra_elt_offset, size);
        fill_mismatched_data(x, start_offset, start_intra_elt_offset, copy_size, buffer, size);
        size = size - copy_size;
        buffer = buffer + copy_size;
        offset = offset + copy_size;
        start_offset++;
    }
    if (size == 0)
    {
        return read_size;
    }
    size_t end_offset = (offset + size - 1) / elt_size;
    size_t end_intra_elt_len = offset + size - end_offset * elt_size;
    if (end_intra_elt_len != 4)
    {
        char *temp_buffer = buffer + end_off * elt_size;
        fill_mismatched_data(x, end_offset, 0, end_intra_elt_len, temp_buffer, size);
        size = size - end_intra_elt_len;
        end_offset--;
    }
    if (size == 0)
    {
        return read_size;
    }
    size_t len = end_off - start_off + 1;

    switch (TYPEOF(x))
    {
    case INTSXP:
        INTEGER_GET_REGION(x, start_off, len, buffer);
        break;
    case LGLSXP:
        LOGICAL_GET_REGION(x, start_off, len, buffer);
        break;
    case REALSXP:
        REAL_GET_REGION(x, start_off, len, buffer);
        break;
    }

    return strlen(selectedText) - offset;
}


void run_fuse()
{
    /*
     Scope_guard const final_action = []{
            free(gb);
            free_list(children);};
    */
    tid = syscall(SYS_gettid);
    operations.getattr = do_getattr;
    operations.readdir = do_readdir;
    operations.read = do_read;
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    fuse_opt_add_arg(&args, "AltPtr");
    fuse_opt_add_arg(&args, "-f");
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, "auto_unmount");
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, "fsname=AltPtr");
    //fuse_opt_add_arg(&args, "/home/jiefei/Documents/mp");
    fuse_opt_add_arg(&args, mount_point.c_str());
    fuse_main(args.argc, args.argv, &operations, NULL);
    //TODO: check if this function will be called after exist
    fuse_opt_free_args(&args);
}

// [[Rcpp::export]]
void run_thread(SEXP path)
{
    thread = new std::thread(run_fuse);
}

// [[Rcpp::export]]
void stop_thread()
{
    kill(tid, SIGTERM);
    thread->join();
}

void set_mountpoint(SEXP path){
    mount_point = CHAR(Rf_asChar(path));
}