
/*
Fuse Functions
struct fuse_operations {
	int (*getattr) (const char *, struct stat *);
	int (*readlink) (const char *, char *, size_t);
	int (*getdir) (const char *, fuse_dirh_t, fuse_dirfil_t);
	int (*mknod) (const char *, mode_t, dev_t);
	int (*mkdir) (const char *, mode_t);
	int (*unlink) (const char *);
	int (*rmdir) (const char *);
	int (*symlink) (const char *, const char *);
	int (*rename) (const char *, const char *);
	int (*link) (const char *, const char *);
	int (*chmod) (const char *, mode_t);
	int (*chown) (const char *, uid_t, gid_t);
	int (*truncate) (const char *, off_t);
	int (*utime) (const char *, struct utimbuf *);
	int (*open) (const char *, struct fuse_file_info *);
	int (*read) (const char *, char *, size_t, off_t,
		     struct fuse_file_info *);
	int (*write) (const char *, const char *, size_t, off_t,
		      struct fuse_file_info *);
	int (*statfs) (const char *, struct statvfs *);
	int (*flush) (const char *, struct fuse_file_info *);
	int (*release) (const char *, struct fuse_file_info *);
	int (*fsync) (const char *, int, struct fuse_file_info *);
	int (*setxattr) (const char *, const char *, const char *, size_t, int);
	int (*getxattr) (const char *, const char *, char *, size_t);
	int (*listxattr) (const char *, char *, size_t);
	int (*removexattr) (const char *, const char *);
	int (*opendir) (const char *, struct fuse_file_info *);
	int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t,
			struct fuse_file_info *);
	int (*releasedir) (const char *, struct fuse_file_info *);
	int (*fsyncdir) (const char *, int, struct fuse_file_info *);
	void *(*init) (struct fuse_conn_info *conn);
	void (*destroy) (void *);
	int (*access) (const char *, int);
	int (*create) (const char *, mode_t, struct fuse_file_info *);
	int (*ftruncate) (const char *, off_t, struct fuse_file_info *);
	int (*fgetattr) (const char *, struct stat *, struct fuse_file_info *);
	int (*lock) (const char *, struct fuse_file_info *, int cmd,
		     struct flock *);
	int (*utimens) (const char *, const struct timespec tv[2]);
	int (*bmap) (const char *, size_t blocksize, uint64_t *idx);
	int (*ioctl) (const char *, int cmd, void *arg,
		      struct fuse_file_info *, unsigned int flags, void *data);
	int (*poll) (const char *, struct fuse_file_info *,
		     struct fuse_pollhandle *ph, unsigned *reventsp);
	int (*write_buf) (const char *, struct fuse_bufvec *buf, off_t off,
			  struct fuse_file_info *);
	int (*read_buf) (const char *, struct fuse_bufvec **bufp,
			 size_t size, off_t off, struct fuse_file_info *);
	int (*flock) (const char *, struct fuse_file_info *, int op);
	int (*fallocate) (const char *, int, off_t, off_t,
			  struct fuse_file_info *);
};

*/

//The function of getattr event will be called when the system tries to get the attributes of the file.
//It's similar to stat function on Linux terminals

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>	//http://man7.org/linux/man-pages/man2/getuid.2.html
#include <sys/types.h> //This and above are for getting uid(userid) and gid(groupid)
#include <time.h>	  //Used to get acess time of files and modification times
#include <string.h>
#include <stdlib.h>

//getattr - similar to stat function in linux
//We will pass in two paramters and retunr an integer
//Param 1: path of file we will get attributes of
//Param 2: stat structure that needs to be that needs to be filled with attributes
//Return 0: if success
//return -1: with errno with correct errorcode
static int p2pGetAttr(const char *path, struct stat *stats)
{

	stats->userID = getuid();			  //stuid is the owner of the file. ->Make this person who mounted the directory.
	stats->groupID = getgid();			  //owner group of the files or directories/subdirectories. ->Make this person who mounted the directory
	stats->acessTIME = time(NULL);		  //last acess time
	stats->modificationTime = time(NULL); //last modification time

	if (strcmp(path, "/") == 0) //Wil run first option if in root
	{
		stats->st_mode = S_IFDIR | 0755; //(check if file or dir) |(permission bits)        st_mode shows if is regular file, directior, other and permission bits of that file.
		stats->st_nlink = 2;			 //Shows number of Hardlinks, Hardlinks, like copying a file but not a copy, a link to original file but modifying hardlink modifies original as well.

		/*Seeing what S_IFDIR retunrs*/
		printf("Testing: \t%i" S_IFDIR);

	} //Reason why twp hardlinks  https://unix.stackexchange.com/questions/101515/why-does-a-new-directory-have-a-hard-link-count-of-2-before-anything-is-added-to/101536#101536
	else
	{
		stats->st_mode = S_IFREG | 0644;
		stats->st_nlink = 1;
		stats->st_size = 1024; //Size of files in bytes
	}

	return 0;
}

//Only need first 3 params - Path of dir
static int p2pDoReadDir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{

	return 0; //returns 0 on success.
}