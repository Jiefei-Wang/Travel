#define FUSE_USE_VERSION 26

#include <fuse/fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <shared_mutex>
#include "read_operations.h"
#include "fuse_filesystem_operations.h"
#include "filesystem_manager.h"
#include "double_key_map.h"
#include "package_settings.h"
#include "utils.h"

static size_t print_counter = 0;

extern double_key_map<fuse_ino_t, std::string, filesystem_file_data> file_list;
std::shared_mutex filesystem_shared_mutex;
/*
Fuse specific objects
*/
static fuse_chan *channel = NULL;
static fuse_session *session = NULL;
static fuse_lowlevel_ops filesystem_operations;

/*
Declare the callback functions
*/
static void filesystem_getattr(fuse_req_t req, fuse_ino_t ino,
                               struct fuse_file_info *fi);
static void filesystem_loopup(fuse_req_t req, fuse_ino_t parent, const char *name);
static void filesystem_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                               off_t off, struct fuse_file_info *fi);
static void filesystem_open(fuse_req_t req, fuse_ino_t ino,
                            fuse_file_info *fi);
static void filesystem_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                            off_t offset, fuse_file_info *fi);

void filesystem_thread_func()
{
    filesystem_operations.lookup = filesystem_loopup;
    filesystem_operations.getattr = filesystem_getattr;
    filesystem_operations.readdir = filesystem_readdir;
    filesystem_operations.open = filesystem_open;
    filesystem_operations.read = filesystem_read;
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    channel = fuse_mount(get_mountpoint().c_str(), &args);
    int err = -1;
    if (channel != NULL)
    {
        session = fuse_lowlevel_new(&args, &filesystem_operations,
                                    sizeof(filesystem_operations), NULL);
        if (session != NULL)
        {
            fuse_session_add_chan(session, channel);
            err = fuse_session_loop(session);
        }
    }
}

void filesystem_stop()
{
    if (channel != NULL && session != NULL)
    {
        debug_print("exiting\n");
        fuse_session_exit(session);
        /*
        clock_t begin_time = clock();
        while (!fuse_session_exited(session))
        {
            if (float(clock() - begin_time) / CLOCKS_PER_SEC > 3)
            {
                Rf_warning("Session exist timeout, force exist instead\n");
            }
        }
        */
        debug_print("Unmounting\n");
        fuse_unmount(get_mountpoint().c_str(), channel);
        debug_print("removing channel\n");
        fuse_session_remove_chan(channel);
        debug_print("destroying session\n");
        fuse_session_destroy(session);
        session = NULL;
        channel = NULL;
    }
}

bool is_filesystem_alive(){
    return session !=NULL && channel !=NULL;

}

std::string get_file_name(fuse_ino_t ino)
{
    if (ino != 1)
    {
        return file_list.get_key2(ino);
    }
    else
    {
        return "/";
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
    if (file_list.has_key1(ino))
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;

        std::shared_lock<std::shared_mutex> shared_lock(filesystem_shared_mutex);
        stbuf->st_size = file_list.get_value_by_key1(ino).file_size;
        return 0;
    }
    return -1;
}

//Assigned
static void filesystem_getattr(fuse_req_t req, fuse_ino_t ino,
                               struct fuse_file_info *fi)
{

    print_to_file("%lu: getattr, ino %lu\n", print_counter++, ino);
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
        print_to_file("%lu: getattr, file name: %s\n", print_counter, get_file_name(ino).c_str());
    }
}

//Assigned
static void filesystem_loopup(fuse_req_t req, fuse_ino_t parent, const char *name)
{

    print_to_file("%lu: lookup, parent %lu, name %s\n", print_counter++, parent, name);
    struct fuse_entry_param e;
    {
        std::shared_lock<std::shared_mutex> shared_lock(filesystem_shared_mutex);
        if (parent != FUSE_ROOT_ID || !file_list.has_key2(name))
        {
            print_to_file("File is not found\n");
            fuse_reply_err(req, ENOENT);
            return;
        }
        print_to_file("File is found\n");

        memset(&e, 0, sizeof(e));
        e.ino = file_list.get_key1(name);
    }
    e.attr_timeout = 0;
    e.entry_timeout = 0;
    fill_file_stat(e.ino, &e.attr);
    int result = fuse_reply_entry(req, &e);
    if (result != 0)
    {
        print_to_file("Fail to send reply, error: %d\n", result);
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

    print_to_file("%lu: readdir, ino %lu\n", print_counter++, ino);
    (void)fi;

    if (ino != 1)
        fuse_reply_err(req, ENOTDIR);
    else
    {
        struct dirbuf b;

        memset(&b, 0, sizeof(b));
        dirbuf_add(req, &b, ".", 1);
        dirbuf_add(req, &b, "..", 1);
        std::shared_lock<std::shared_mutex> shared_lock(filesystem_shared_mutex);
        for (auto i = file_list.begin_key(); i != file_list.end_key(); ++i)
        {
            print_to_file("File Added: %s\n", i->second.c_str());
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
    print_to_file("%lu: open, ino %lu\n", print_counter++, ino);
    {
        std::shared_lock<std::shared_mutex> shared_lock(filesystem_shared_mutex);
        if (!file_list.has_key1(ino))
        {
            fuse_reply_err(req, ENOENT);
            return;
        }
    }
    if ((fi->flags & 3) != O_RDONLY)
    {
        fuse_reply_err(req, EACCES);
        return;
    }
    print_to_file("%lu: open, name %s\n", print_counter++, get_file_name(ino).c_str());
    fuse_reply_open(req, fi);
}

//assigned
static void filesystem_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                            off_t offset, fuse_file_info *fi)
{
    print_to_file("%lu: Read, ino %lu, name %s\n", print_counter++, ino, file_list.get_key2(ino).c_str());

    std::shared_lock<std::shared_mutex> shared_lock(filesystem_shared_mutex);
    const size_t file_size = file_list.get_value_by_key1(ino).file_size;
    //Out-of-bound check;
    if ((size_t)offset + size > file_size)
    {
        if ((size_t)offset >= file_size)
        {
            fuse_reply_buf(req, NULL, 0);
            return;
        }
        else
        {
            size = file_size - offset;
        }
    }
    std::unique_ptr<char[]> buffer(new char[size]);
    filesystem_file_data &file_data = file_list.get_value_by_key1(ino);
    size_t read_size = general_read_func(file_data, buffer.get(), offset, size);
    print_to_file("%lu: Request read %llu, true read:%llu", size, read_size);
    fuse_reply_buf(req, buffer.get(), read_size);
}
