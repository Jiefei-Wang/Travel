#define FUSE_USE_VERSION 26

#include <fuse/fuse_lowlevel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


#include <thread>
#include <memory>
#include <map>
#include "doubleKeyMap.h"
#include "package_settings.h"

static size_t print_counter = 0;
static fuse_ino_t file_inode_counter = 1;


struct fuse_file_data;
typedef int (*file_data_func)(fuse_file_data& file_data, char* buffer, size_t offset, size_t size); 
struct fuse_file_data
{
    unsigned long long file_size;
    //This function do not need to do the out-of-bound check
    file_data_func data_func;
};



double_key_map<fuse_ino_t, std::string, fuse_file_data> file_list;

void add_virtual_file(std::string name, fuse_file_data file_data)
{
    file_inode_counter++;
    file_list.insert(file_inode_counter, name, file_data);
}

std::string get_file_name(fuse_ino_t ino){
    if(ino!=1){
        return file_list.get_key2(ino);
    }else{
        return "/";
    }
}


static int hello_stat(fuse_ino_t ino, struct stat *stbuf)
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
        stbuf->st_size = file_list.get_value_by_key1(ino).file_size;
        return 0;
    }
    return -1;
}

//Assigned
static void hello_ll_getattr(fuse_req_t req, fuse_ino_t ino,
                             struct fuse_file_info *fi)
{
    
    printf("%lu: getattr, ino %lu\n", print_counter++, ino);
    struct stat stbuf;
    (void)fi;
    memset(&stbuf, 0, sizeof(stbuf));
    if (hello_stat(ino, &stbuf) == -1)
    {
        fuse_reply_err(req, ENOENT);
    }
    else
    {
        fuse_reply_attr(req, &stbuf, 1.0);
        printf("%lu: getattr, file name: %s\n", print_counter, get_file_name(ino).c_str());
    }
}

//Assigned
static void hello_ll_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{
    
    printf("%lu: lookup, parent %lu, name %s\n", print_counter++, parent, name);
    struct fuse_entry_param e;
    
    if (parent != FUSE_ROOT_ID || !file_list.has_key2(name))
    {
        fuse_reply_err(req, ENOENT);
        return;
    }
    
    memset(&e, 0, sizeof(e));
    e.ino = file_list.get_key1(name);
    e.attr_timeout = 1.0;
    e.entry_timeout = 1.0;
    hello_stat(e.ino, &e.attr);
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
static void hello_ll_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
                             off_t off, struct fuse_file_info *fi)
{
    
    printf("%lu: readdir, ino %lu\n", print_counter++, ino);
    (void)fi;
    
    if (ino != 1)
        fuse_reply_err(req, ENOTDIR);
    else
    {
        struct dirbuf b;
        
        memset(&b, 0, sizeof(b));
        dirbuf_add(req, &b, ".", 1);
        dirbuf_add(req, &b, "..", 1);
        for(auto i = file_list.begin_key();i!=file_list.end_key();++i){
            dirbuf_add(req, &b, i->second.c_str(), i->first);
        }
        reply_buf_limited(req, b.p, b.size, off, size);
        free(b.p);
    }
}

//Assigned
static void hello_ll_open(fuse_req_t req, fuse_ino_t ino,
                          fuse_file_info *fi)
{
    printf("%lu: open, ino %lu\n", print_counter++, ino);
    if(!file_list.has_key1(ino)){
        fuse_reply_err(req, ENOENT);
        return;
    }
    if((fi->flags & 3) != O_RDONLY){
        fuse_reply_err(req, EACCES);
        return;
    }
    printf("%lu: open, name %s\n", print_counter, get_file_name(ino).c_str());
    fuse_reply_open(req, fi);
}

//assigned
static void hello_ll_read(fuse_req_t req, fuse_ino_t ino, size_t size,
                          off_t offset, fuse_file_info *fi)
{
    printf("%lu: Read, ino %lu, name %s\n", print_counter++, ino, file_list.get_key2(ino).c_str());
    const size_t file_size = file_list.get_value_by_key1(ino).file_size;
    //Out-of-bound check;
    if((size_t)offset + size > file_size){
        if((size_t)offset >= file_size){
            fuse_reply_buf(req, NULL, 0);
            return;
        }else{
            size = file_size - offset;
        }
    }
    std::unique_ptr<char[]> buffer(new char[size]); 
    fuse_file_data& file_data = file_list.get_value_by_key1(ino);
    file_data.data_func(file_data,buffer.get(), offset, size);
    fuse_reply_buf(req, buffer.get(), size);
}






static fuse_lowlevel_ops hello_ll_oper;
static std::unique_ptr<std::thread> filesystem_thread(nullptr);
static fuse_chan* ch;
static fuse_session* se;

int fake_data_func(fuse_file_data& file_data, char* buffer, size_t offset, size_t size){
    const char data[] = "test data";
    size_t fake_data_size = sizeof(data)-1;
    for(size_t i = 0; i<size;i++){
        buffer[i] = data[(i+offset)%fake_data_size];
    }
    return 0;
}


int filesystem_thread_func()
{
    hello_ll_oper.lookup = hello_ll_lookup;
    hello_ll_oper.getattr = hello_ll_getattr;
    hello_ll_oper.readdir = hello_ll_readdir;
    hello_ll_oper.open = hello_ll_open;
    hello_ll_oper.read = hello_ll_read;
    struct fuse_args args = FUSE_ARGS_INIT(0, NULL);
    ch=fuse_mount(get_mountpoint().c_str(), &args);
    int err = -1;
    
    if (ch!= NULL)
    {
        se=fuse_lowlevel_new(&args, &hello_ll_oper,
                             sizeof(hello_ll_oper), NULL);
        fuse_session_add_chan(se, ch);
        err = fuse_session_loop(se);
        
    }
    return err ? 1 : 0;
}

// [[Rcpp::export]]
void C_stop_filesystem_thread(){
    if(filesystem_thread!=nullptr){
        fuse_session_remove_chan(ch);
        fuse_session_destroy(se);
        fuse_unmount(get_mountpoint().c_str(), ch);
        printf("unmount\n");
    }
}


// [[Rcpp::export]]
void C_run_filesystem_thread(){
	fuse_file_data file_data1;
	file_data1.data_func= fake_data_func;
	file_data1.file_size= 100;
	add_virtual_file("test1", file_data1);

	fuse_file_data file_data2;
	file_data1.data_func= fake_data_func;
	file_data1.file_size= 200;
	add_virtual_file("test2", file_data1);
	
	std::thread* thread = new std::thread(filesystem_thread_func);
	filesystem_thread.reset(thread);
}