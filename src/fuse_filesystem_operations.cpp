#define FUSE_USE_VERSION 26

#include <fuse_lowlevel.h>
#include <Rcpp.h>
#include <chrono>
#include <future>
#include <thread>
#include <shared_mutex>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <memory>
#include <sys/mman.h>

#define FILE_SIZE 16777216
#define FILE_NAME "inode2"
/*
Fuse specific objects
*/
static fuse_chan *channel = NULL;
static fuse_session *session = NULL;
static fuse_lowlevel_ops filesystem_operations;

static int fill_file_stat(fuse_ino_t ino, struct stat *stbuf)
{
    stbuf->st_ino = ino;
    if (ino == 1)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (ino == 2)
    {
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_size = FILE_SIZE;
        return 0;
    }
    return -1;
}

//Assigned
static void filesystem_getattr(fuse_req_t req, fuse_ino_t ino,
                               struct fuse_file_info *fi)
{

    struct stat stbuf;
    memset(&stbuf, 0, sizeof(stbuf));
    if (fill_file_stat(ino, &stbuf) == -1)
    {
        fuse_reply_err(req, ENOENT);
    }
    else
    {
        fuse_reply_attr(req, &stbuf, 1.0);
    }
}

//Assigned
static void filesystem_loopup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    struct fuse_entry_param e;
    if (parent != FUSE_ROOT_ID || strcmp(name,FILE_NAME)!=0)
    {
        fuse_reply_err(req, ENOENT);
        return;
    }
    memset(&e, 0, sizeof(e));
    e.ino = 2;
    e.attr_timeout = 0;
    e.entry_timeout = 0;
    fill_file_stat(e.ino, &e.attr);
    fuse_reply_entry(req, &e);
}

struct dirbuf
{
    char *p;
    size_t size;
};

static void dirbuf_add(fuse_req_t req, struct dirbuf *b, const char *name,
                       fuse_ino_t ino)
{
    struct stat stbuf;
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;

    size_t oldsize = b->size;
    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    b->p = (char *)realloc(b->p, b->size);
    fuse_add_direntry(req, b->p + oldsize, b->size - oldsize, name, &stbuf,
                      b->size);
}

#define min(x, y) ((x) < (y) ? (x) : (y))

static int reply_buf_limited(fuse_req_t req, const char *buf, size_t bufsize,
                             off_t off, size_t maxsize)
{
    if ((size_t)off < bufsize)
        return fuse_reply_buf(req, buf + off,
                              min(bufsize - off, maxsize));
    else
        return fuse_reply_buf(req, NULL, 0);
}

//Assigned
static void filesystem_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                               off_t off, struct fuse_file_info *fi)
{
    if (ino != 1)
        fuse_reply_err(req, ENOTDIR);
    else
    {
        struct dirbuf b;

        memset(&b, 0, sizeof(b));
        dirbuf_add(req, &b, ".", 1);
        dirbuf_add(req, &b, "..", 1);
        dirbuf_add(req, &b, FILE_NAME, 2);
        reply_buf_limited(req, b.p, b.size, off, size);
        free(b.p);
    }
}

//Assigned
static void filesystem_open(fuse_req_t req, fuse_ino_t ino,
                            fuse_file_info *fi)
{

    if (ino != 2)
    {
        fuse_reply_err(req, ENOENT);
        return;
    }
    if ((fi->flags & O_ACCMODE) != O_RDWR &&
        (fi->flags & O_ACCMODE) != O_RDONLY)
    {
        fuse_reply_err(req, EACCES);
        return;
    }
    fi->nonseekable = false;
    fuse_reply_open(req, fi);
}

//Data buffer
static size_t buffer_size = 1024;
std::unique_ptr<char[]> buffer(new char[buffer_size]);
static void filesystem_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                            off_t offset, fuse_file_info *fi)
{
    buffer.reset(new char[size]);
    fuse_reply_buf(req, buffer.get(), size);
    return;
}
int ct = 0;
static void filesystem_write(fuse_req_t req, fuse_ino_t ino, const char *buffer,
                             size_t buffer_length, off_t offset, struct fuse_file_info *fi)
{
    fuse_reply_write(req, buffer_length);
    char *block_ptr = (char *)malloc(4096);
}

/*
========================================================
Filesystem exported APIs
========================================================
*/
// mount point
static std::string mount_point;
// [[Rcpp::export]]
void C_set_mountpoint(SEXP path)
{
    mount_point = CHAR(Rf_asChar(path));
}
std::string get_mountpoint(){
    return mount_point;
}
// [[Rcpp::export]]
SEXP C_get_mountpoint(){
	return Rf_mkString(get_mountpoint().c_str());
}

// [[Rcpp::export]]
void filesystem_thread_func()
{
    filesystem_operations.lookup = filesystem_loopup;
    filesystem_operations.getattr = filesystem_getattr;
    filesystem_operations.readdir = filesystem_readdir;
    filesystem_operations.open = filesystem_open;
    filesystem_operations.read = filesystem_read;
    filesystem_operations.write = filesystem_write;
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    channel = fuse_mount(get_mountpoint().c_str(), &args);
    if (channel != NULL)
    {
        session = fuse_lowlevel_new(&args, &filesystem_operations,
                                    sizeof(filesystem_operations), NULL);
        if (session != NULL)
        {
            fuse_session_add_chan(session, channel);
            fuse_session_loop(session);
            fuse_session_remove_chan(channel);
        }
        fuse_session_destroy(session);
    }
    session = NULL;
    channel = NULL;
    fuse_opt_free_args(&args);
}


static std::unique_ptr<std::thread> filesystem_thread(nullptr);

// [[Rcpp::export]]
void C_run_filesystem_thread()
{
  filesystem_thread.reset(new std::thread(filesystem_thread_func));
}


// [[Rcpp::export]]
int C_mmp(SEXP path, size_t length, Rcpp::NumericVector ind, Rcpp::NumericVector v)
{
    std::string file_full_path = Rcpp::as<std::string>(path);
    int fd = open(file_full_path.c_str(), O_RDWR); 
    if (fd == -1)
    {
        return -1;
    }
    int *ptr = (int *)mmap(NULL, length*4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == (void *)-1)
    {
        close(fd);
        return -2;
    }
    for(size_t i=0;i<length;i++){
        Rprintf("%llu: %llu\n",i, (size_t)ind(i)-1);
        if(length<=ind(i)){
            Rf_error("error");
        }
        ptr[(size_t)ind(i)-1]=v(i);
    }
    return 0;
}