#define FUSE_USE_VERSION 30
#include <fuse.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <thread>
#include <signal.h>
#ifndef _WIN32
#include <sys/syscall.h>
#endif

#include <Rcpp.h>
#include <map>
#include <mutex>
#include "utils.h"
#include "package_settings.h"
#include "fuse.h"
#include "altrep_operations.h"

#define BUFFER_SIZE (1024 * 1024)

struct file_info
{
    SEXP x;
    SEXP x_internal;
    size_t size;
    size_t elt_size;
};

using std::map;
using std::string;
typedef std::pair<const string, file_info> value_type;

//A struct to store all fuse operations
static struct fuse_operations operations;
//A map to the altrep objects
static size_t counter = 0;
static map<const string, file_info> altrep_map;
//A shared mutex that is used for accessing map
static std::mutex mutex;

//process id and thread that the fuse will be running on
pid_t tid;
std::thread *thread = nullptr;

#define HAS_KEY(path) (altrep_map.find(path) != altrep_map.end())
#define GET_VALUE(path) altrep_map.at(path)

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
    mutex.lock();
    if (name != "" && !HAS_KEY(name))
    {
        mutex.unlock();
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
        st->st_size = GET_VALUE(name).size;
    }
    mutex.unlock();
    return 0;
}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
    debug_print("--> Getting The List of Files of %s\n", path);

    filler(buffer, ".", NULL, 0);  // Current Directory
    filler(buffer, "..", NULL, 0); // Parent Directory

    if (strcmp(path, "/") == 0) // If the user is trying to show the files/directories of the root directory show the following
    {
        mutex.lock();
        for (auto i : altrep_map)
        {
            filler(buffer, i.first.c_str(), NULL, 0);
        }
        mutex.unlock();
    }
    return 0;
}
// Fill the data that cannot be completely represent by an element of x
// e.g. x is an integer vector, offset = 1, size = 2. only the second and third bytes of x[1] is required
#define fill_mismatched_data(x, i, intra_elt_offset, copy_size, buffer, buffer_size) \
    switch (TYPEOF(x))                                                               \
    {                                                                                \
    case INTSXP:                                                                     \
    {                                                                                \
        int elt = INTEGER_ELT(x, i);                                                 \
        memcpy(buffer, ((char *)&elt) + intra_elt_offset, copy_size);                \
        break;                                                                       \
    }                                                                                \
    case LGLSXP:                                                                     \
    {                                                                                \
        int elt = LOGICAL_ELT(x, i);                                                 \
        memcpy(buffer, ((char *)&elt) + intra_elt_offset, copy_size);                \
        break;                                                                       \
    }                                                                                \
    case REALSXP:                                                                    \
    {                                                                                \
        double elt = REAL_ELT(x, i);                                                 \
        memcpy(buffer, ((char *)&elt) + intra_elt_offset, copy_size);                \
        break;                                                                       \
    }                                                                                \
    }

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
    debug_print("--> Reading file %s, offset: %llu, size: %llu\n", path, offset, size);

    string name(path + 1L);
    SEXP x_internal;
    size_t elt_size;
    size_t file_size;
    mutex.lock();
    bool key_exist = HAS_KEY(name);
    if (key_exist)
    {
        x_internal = GET_VALUE(name).x_internal;
        elt_size = GET_VALUE(name).elt_size;
        file_size = GET_VALUE(name).size;
    }
    mutex.unlock();
    if (!key_exist)
    {
        return -1;
    }

    if (offset + size > file_size)
    {
        if ((size_t)offset < file_size)
            size = file_size - offset;
        else
            size = 0;
        debug_print("Size truncated to %llu\n", size);
    }
    if (size == 0)
        return 0;
    size_t read_size = size;

    size_t start_offset = offset / elt_size;
    size_t start_intra_elt_offset = offset - start_offset * elt_size;
    if (start_intra_elt_offset != 0)
    {
        size_t copy_size = std::max(elt_size - start_intra_elt_offset, size);
        debug_print("Filling start %llu,%llu,%llu,%llu\n", start_offset, start_intra_elt_offset, copy_size, size);
        fill_mismatched_data(x_internal, start_offset, start_intra_elt_offset, copy_size, buffer, size);
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
        char *temp_buffer = buffer + end_offset * elt_size;
        debug_print("Filling end %llu,%llu,%llu,%llu\n", end_offset, end_intra_elt_len, size);
        fill_mismatched_data(x_internal, end_offset, 0, end_intra_elt_len, temp_buffer, size);
        size = size - end_intra_elt_len;
        end_offset--;
    }
    if (size == 0)
    {
        return read_size;
    }
    size_t len = end_offset - start_offset + 1;

     debug_print("Filling rest %llu,%llu\n", start_offset, len);
    switch (TYPEOF(x_internal))
    {
    case INTSXP:
        INTEGER_GET_REGION(x_internal, start_offset, len, (int *)buffer);
        break;
    case LGLSXP:
        LOGICAL_GET_REGION(x_internal, start_offset, len, (int *)buffer);
        break;
    case REALSXP:
        REAL_GET_REGION(x_internal, start_offset, len, (double *)buffer);
        break;
    }

    return read_size;
}

struct fuse_args args;
void run_fuse()
{
    tid = syscall(SYS_gettid);
    operations.getattr = do_getattr;
    operations.readdir = do_readdir;
    operations.read = do_read;
    fuse_main(args.argc, args.argv, &operations, NULL);
}

// [[Rcpp::export]]
void C_run_fuse_thread()
{
    if (thread != nullptr)
    {
        Rf_error("The fuse has been running!\n");
    }
    args = FUSE_ARGS_INIT(0, NULL);
    fuse_opt_add_arg(&args, "AltPtr");
    fuse_opt_add_arg(&args, "-f");
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, "auto_unmount");
    fuse_opt_add_arg(&args, "-o");
    fuse_opt_add_arg(&args, "fsname=AltPtr");
    fuse_opt_add_arg(&args, get_mountpoint().c_str());
    thread = new std::thread(run_fuse);
}

// [[Rcpp::export]]
void C_stop_fuse_thread()
{
    if (thread == nullptr)
    {
        return;
    }
    fuse_opt_free_args(&args);
    kill(tid, SIGTERM);
    thread->join();
    delete thread;
    thread = nullptr;
}

// [[Rcpp::export]]
void C_add_altrep_to_fuse(SEXP x, SEXP name)
{
    add_altrep_to_fuse(x, name);
}

string add_altrep_to_fuse(SEXP x, SEXP name)
{
    string altrep_name;

    if (name != R_NilValue)
    {
        altrep_name = CHAR(Rf_asChar(name));
    }
    else
    {
        altrep_name = "AP_" + std::to_string(counter);
        counter++;
    }
    file_info info = {x, GET_ALT_DATA(x), get_object_size(x), get_type_size(TYPEOF(x))};
    mutex.lock();
    altrep_map.insert(value_type(altrep_name, info));
    mutex.unlock();
    return altrep_name;
}

void remove_altrep_from_fuse(SEXP name)
{
    string altrep_name(CHAR(Rf_asChar(name)));
    mutex.lock();
    if (HAS_KEY(altrep_name))
    {
        altrep_map.erase(altrep_name);
    }
    mutex.unlock();
}

// [[Rcpp::export]]
SEXP C_list_altrep()
{
    mutex.lock();
    Rcpp::List x;
    for (auto i : altrep_map)
    {
        x.push_back((SEXP)i.second.x, i.first);
    }
    mutex.unlock();
    return x;
}