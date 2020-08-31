#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>


static int do_getattr( const char *path, struct stat *st )
{
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_atime = time( NULL );
    st->st_mtime = time( NULL );
    if ( strcmp( path, "/" ) == 0 )
    {
        st->st_mode = S_IFDIR | 0755;
        st->st_nlink = 2;
    }
    else
    {
        st->st_mode = S_IFREG | 0644;
        st->st_nlink = 1;
        st->st_size = 1024;
    }
    
    return 0;
}