#ifndef _WIN32
#define FUSE_USE_VERSION 26

#include <fuse_lowlevel.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <memory>
#include "read_write_operations.h"
#include "filesystem_operations.h"
#include "filesystem_manager.h"
#include "double_key_map.h"
#include "package_settings.h"
#include "utils.h"

static size_t print_counter = 0;
/*
Fuse specific objects
*/
static fuse_chan *channel = NULL;
static fuse_session *session = NULL;
static fuse_lowlevel_ops filesystem_operations;

static std::string root_path = "/";
static const std::string& get_file_name(fuse_ino_t ino)
{
    if (ino != 1)
    {
        return get_filesystem_file_name(ino);
    }
    else
    {
        return root_path;
    }
}
static int fill_file_stat(fuse_ino_t ino, struct stat *stbuf)
{
    stbuf->st_ino = ino;
    if (ino == 1)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (has_filesystem_file(ino))
    {
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;

        stbuf->st_size = get_filesystem_file_data(ino).file_size;
        return 0;
    }
    return -1;
}

//Assigned
static void filesystem_getattr(fuse_req_t req, fuse_ino_t ino,
                               struct fuse_file_info *fi)
{

    filesystem_log("%lu: getattr, ino %lu\n", print_counter++, ino);
    struct stat stbuf;
    (void)fi;
    memset(&stbuf, 0, sizeof(stbuf));
    if (fill_file_stat(ino, &stbuf) == -1)
    {
        fuse_reply_err(req, ENOENT);
    }
    else
    {
        fuse_reply_attr(req, &stbuf, 1.0);
        filesystem_log("%lu: getattr, file name: %s\n", print_counter, get_file_name(ino).c_str());
    }
}

//Assigned
static void filesystem_loopup(fuse_req_t req, fuse_ino_t parent, const char *name)
{

    filesystem_log("%lu: lookup, parent %lu, name %s\n", print_counter++, parent, name);
    struct fuse_entry_param e;
    {
        if (parent != FUSE_ROOT_ID || !has_filesystem_file(name))
        {
            filesystem_log("File is not found\n");
            fuse_reply_err(req, ENOENT);
            return;
        }
        filesystem_log("File is found\n");

        memset(&e, 0, sizeof(e));
        e.ino = get_filesystem_file_inode(name);
    }
    e.attr_timeout = 0;
    e.entry_timeout = 0;
    fill_file_stat(e.ino, &e.attr);
    int result = fuse_reply_entry(req, &e);
    if (result != 0)
    {
        filesystem_log("Fail to send reply, error: %d\n", result);
    }
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
    size_t oldsize = b->size;
    b->size += fuse_add_direntry(req, NULL, 0, name, NULL, 0);
    b->p = (char *)realloc(b->p, b->size);
    memset(&stbuf, 0, sizeof(stbuf));
    stbuf.st_ino = ino;
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

    filesystem_log("%lu: readdir, ino %lu\n", print_counter++, ino);

    if (ino != 1)
        fuse_reply_err(req, ENOTDIR);
    else
    {
        struct dirbuf b;

        memset(&b, 0, sizeof(b));
        dirbuf_add(req, &b, ".", 1);
        dirbuf_add(req, &b, "..", 1);
        for (auto i = filesystem_file_begin(); i != filesystem_file_end(); ++i)
        {
            filesystem_log("File Added: %s\n", i->second.c_str());
            dirbuf_add(req, &b, i->second.c_str(), i->first);
        }
        reply_buf_limited(req, b.p, b.size, off, size);
        free(b.p);
    }
}

//Assigned
static void filesystem_open(fuse_req_t req, fuse_ino_t ino,
                            fuse_file_info *fi)
{
    size_t current_counter = print_counter++;
    filesystem_log("%lu: open, ino %lu\n", current_counter, ino);
    {
        if (!has_filesystem_file(ino))
        {
            fuse_reply_err(req, ENOENT);
            return;
        }
    }
    filesystem_log("%lu: open, name %s, flag %d\n", current_counter, get_file_name(ino).c_str(), fi->flags);
    if ((fi->flags & O_ACCMODE) != O_RDWR&&
        (fi->flags & O_ACCMODE) != O_RDONLY)
    {
    filesystem_log("%lu: Access denied\n", current_counter);
        fuse_reply_err(req, EACCES);
        return;
    }
    fuse_reply_open(req, fi);
}

//Data buffer
static Unique_buffer buffer;
static void filesystem_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                            off_t offset, fuse_file_info *fi)
{
    size_t current_counter = print_counter++;
    Filesystem_file_data &file_data = get_filesystem_file_data(ino);
    size_t &file_size = file_data.file_size;
    size_t desired_size = get_file_read_size(file_size, offset, size);
    filesystem_log("%llu: Read, ino %lu, name %s, offset:%llu, size:%llu\n",
                   current_counter, ino, get_filesystem_file_name(ino).c_str(),
                   offset, desired_size);
    if (size == 0){
        fuse_reply_buf(req, NULL, 0);
        return;
    }
    buffer.reserve(desired_size);
    size_t read_size = general_read_func(file_data, buffer.get(),
                                         offset,
                                         desired_size);
    int status = fuse_reply_buf(req, buffer.get(), read_size);
    if (status != 0)
    {
        filesystem_log("%llu: error in read! code %d\n", current_counter, status);
    }
    if (desired_size != read_size)
    {
        filesystem_log("%llu: expect size and read size do not match!\n");
    }
   buffer.release();
}

static void filesystem_write(fuse_req_t req, fuse_ino_t ino, const char *buffer,
                             size_t buffer_size, off_t offset, struct fuse_file_info *fi)
{
    Filesystem_file_data &file_data = get_filesystem_file_data(ino);
	size_t &file_size = file_data.file_size;
	size_t write_size = get_file_read_size(file_size, offset, buffer_size);
	size_t true_write_size = general_write_func(file_data, buffer, offset, write_size);
	filesystem_log("file_size:%llu, offset:%llu, request write %llu, matched write size:%llu, true write size:%llu\n", 
	file_size, offset, buffer_size, write_size,true_write_size);
    fuse_reply_write(req, true_write_size);
}

/*
========================================================
Filesystem exported APIs
========================================================
*/

void filesystem_thread_func(int *thread_status)
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
            int status = fuse_session_loop(session);
            if (status > 0)
            {
                *thread_status = 0;
            }
            else
            {
                *thread_status = status;
            }
            fuse_session_remove_chan(channel);
        }
        fuse_session_destroy(session);
#ifdef __APPLE__
        fuse_unmount(get_mountpoint().c_str(), channel);
#endif
    }
    if (channel == NULL)
    {
        *thread_status = 1000;
    }
    else
    {
        if (session == NULL)
            *thread_status = 1001;
    }
    session = NULL;
    channel = NULL;
    fuse_opt_free_args(&args);
}

void filesystem_stop()
{
    if (channel != NULL)
    {
        filesystem_print("Unmounting\n");
#ifdef __APPLE__
        unmount(get_mountpoint().c_str(), MNT_FORCE);
#else
        fuse_unmount(get_mountpoint().c_str(), channel);
#endif
    }
}

bool is_filesystem_alive()
{
    return session != NULL && channel != NULL;
}

std::string get_error_message(int status)
{
    switch (status)
    {
    case 0:
        return "success";
    case 1000:
        return "Cannot build channel";
    case 1001:
        return "Cannot build session";
    default:
        return "Unknown error";
    }
}

#endif