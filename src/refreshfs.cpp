#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>



#include <iostream>
#include <vector>
#include <opendht.h>
#include <set>
#include <algorithm>
#include <fstream>

#include <map>

#include<thread>
#include<mutex>

std::mutex mtx;

using std::cout;
using std::endl;

#define PATH_MAX 4096

struct refreshfs_dirp
{
	DIR *dp;
	struct dirent *entry;
	off_t offset;
};

static struct mountpoint
{
	int fd;
	struct refreshfs_dirp *dir;
	char *path;
} mountpoint;




dht::DhtRunner node;
std::set<std::string> listOfFiles;
std::set<std::string> listOfContent;
std::set<std::string> localFiles;
std::map<std::string,std::string> contentMap;
bool wait = 0;

int order = 0;


bool inLocalFiles(std::string path){
	bool isIn = localFiles.find(path) != localFiles.end();
	return isIn;
}



std::string dataRetrieved;
void translateDHTEntry() 
{
	mtx.lock();
	for(auto i = listOfFiles.begin(); i != listOfFiles.end(); ++i)//Iterate throught list of files
	{

		//put in order the content in the listofcontent
		node.get(*i, 
		[&](const std::vector<std::shared_ptr<dht::Value>>& values)  
		{

			
			// Callback called when values are found
			for (const auto& value : values)
			{
				std::stringstream mystream;
				std::string dataAsString;
				mystream << *value;
				dataAsString = mystream.str();

				dataAsString = dataAsString.substr(dataAsString.find("data:") + 7);
				dataAsString.pop_back();

				int len = dataAsString.length();
				std::string newString;
				for(int i=0; i< len; i+=2)
				{
					std::string byte = dataAsString.substr(i,2);
					char chr = (char) (int)strtol(byte.c_str(), NULL, 16);
					newString.push_back(chr);
				}

				//cout << "+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++FINAL: " << newString << endl << endl; 
				dataRetrieved = newString;
				//cout << "This is the content of the File: \n\n" << dataRetrieved <<endl;
				//listOfContent.insert(dataRetrieved);
				//std::pair<std::map<std::string,std::string>::iterator,bool>ret;
				contentMap.insert(std::pair<std::string,std::string>(*i,dataRetrieved));
				/*
				if(ret.second == false){
					cout << "For path: " << *i << " content already exists with value " << ret.first->second << endl;
				}
				*/
			}

			// usleep(1000);

			return true; // return false to stop the search
		},
		[](bool success) {
			std::cout << "\n\n\n\nRetrive, Translate, Scan CONTENT in ODHT..." << (success ? "success" : "failure") << std::endl;
			wait = 1;
		});
	}

	mtx.unlock();
	return;
	//return 0;
}






int translateListOfFiles() 
{
	// listOfFiles.clear();
	mtx.lock();
	node.get((char*)"LIST_OF_FILES", 
	[&](const std::vector<std::shared_ptr<dht::Value>>& values)  
	{

		
		// Callback called when values are found
		for (const auto& value : values)
		{
			std::stringstream mystream;
			std::string dataAsString;
			// std::cout << value.ValueType << endl;



			mystream << *value;
			dataAsString = mystream.str();

			dataAsString = dataAsString.substr(dataAsString.find("data:") + 7);
			dataAsString.pop_back();

			//Stop Conversion
			
			int len = dataAsString.length();
			std::string newString;
			for(int i=0; i< len; i+=2)
			{
				std::string byte = dataAsString.substr(i,2);
				char chr = (char) (int)strtol(byte.c_str(), NULL, 16);
				newString.push_back(chr);
			}			
			listOfFiles.insert(newString);
		}

		return true; // return false to stop the search
    },
		[](bool success) {
			std::cout << "\n\nRetrive, Translate, Scan Filenames in ODHT..." << (success ? "success" : "failure") << std::endl;
			wait = 1;
		});

	mtx.unlock();
	
	return 0;
}




//Initially no entries in each of the arrays so index -1
int do_release(const char *path, struct fuse_file_info *fi)
{
	char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);


	order++;
	printf("in do_release--------------------------------------------------------------------------------%i\n", order);



    return close(fi->fh);
}



int do_open(const char *path, struct fuse_file_info *fi)
{

	order++;
	printf("------------------------------------------------------------------------------do_open: %i\n", order);

    int retstat = 0;
    int fd;
    
	char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
    
    // if the open call succeeds, my retstat is the file descriptor,
    // else it's -errno.  I'm making sure that in that case the saved
    // file descriptor is exactly -1.

	
	printf("in do_open path: %s", path);
    fd = open(fpath, fi->flags);



	//try open
	//check for flag
	//if not there get from dht and make a file
	//open again


    if (fd < 0)
	{
		perror("error in do_open");
		retstat = -errno;
	}

    fi->fh = fd;

    
    return retstat;
}


int do_opendir(const char *path, struct fuse_file_info *fi)
{
	order++;
	printf("--------------------------------------------------------------------do_opendir: %i\n", order);

    DIR *dp;
    int retstat = 0;
    char fpath[PATH_MAX];
    
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);

    // since opendir returns a pointer, takes some custom handling of
    // return status.

	// ++path;
	// strncpy(fpath, mountpoint.path, PATH_MAX);
	// strncat(fpath, path, PATH_MAX);

	// printf("in do_opendir path: %s\n", path);
	
	printf("in do_opendir path: %s\n", path);
    dp = opendir(fpath);
    // log_msg("    opendir returned 0x%p\n", dp);
    if (dp == NULL)
	{
		 perror("in do_opendir dp is null\n");
		retstat = -errno;
	}
    fi->fh = (intptr_t) dp;
    
	if(fi->fh == NULL)
	{
		printf("in do_opendir fi->fh is null\n");
	}
    // log_fi(fi);
    
    return retstat;
}

// ... //


// Fill in s1 and s2 with values
std::set<std::string> newFiles;


std::set<std::string> PlistOfFiles;
bool updateInProgress = false;

static int do_getattr(const char *path, struct stat *st)
{



	std::string pathStrigify = path;
	if(listOfFiles.find(pathStrigify) != listOfFiles.end()    )
	{
		cout << "Current path provided is in our ODHT\n";
	}

	
	
	//bool contentRetrieved = translateDHTEntry(path); //update list


	/*
	if(newFiles.size() > 0 && !updateInProgress)
	{
		updateInProgress = true;


		cout << "...files construction beginning...\n";
		int counteri = 0;
		for(auto i = newFiles.begin(); i != newFiles.end(); ++i)
		{
			
			std::ofstream myfile;
			//cout << "1" << endl;
			std::string newFileName = *i; 
			char fpath[PATH_MAX];
			strncpy(fpath, mountpoint.path, PATH_MAX);
			strncat(fpath, newFileName.c_str(), PATH_MAX);
			//cout << "fpath: " << fpath << endl;
			myfile.open (fpath);
			//cout << "2" << endl;
			int counterj = 0;
			for(auto j = listOfContent.begin(); j != listOfContent.end(); ++j)
			{
				//cout << *j << ", ";
				if(counteri == counterj){
					std::string content = *j;
					myfile << content;
				}
				counterj++;
			}
			//std::string content = *listOfContent.begin();
			//myfile << content;
			//myfile << "GET_FROM_DHT";
			//cout << "3" << endl;
			myfile.close();
			//cout << "4" << endl;
			counteri++;
		}
		cout << "...fileconstruction finished.\n";
		updateInProgress = false;
		PlistOfFiles = listOfFiles;
	}
	*/

	order = 0;
	printf("------------------------------------------------------getattr: %i\n", order);
	printf("File is %s\n", path);

	int returnStatus;


	const bool isINFILES = listOfFiles.find(path) != listOfFiles.end();
	st->st_uid = getuid();
	st->st_gid = getgid();
	if(strcmp(path,"/") == 0){
		st->st_mode = __S_IFDIR | 0755;
		st->st_nlink = 2;
		return 0;
	}else if(isINFILES ){
		cout << "IS IN ____OPENDHT" << endl;
		
		//st->st_atim = time(now);
		//st->st_mtim = time(now);
		st->st_mode = __S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
		return 0;
	}else{
		cout << "IS NOT ____OPENDHT" << endl;
		return -ENONET;
	}

	char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);


	returnStatus = stat(fpath, st);


	if (returnStatus < 0)
	{
		perror("Something went wrong in do_getattr: \n");
		returnStatus = -errno;
	}

	if (S_ISDIR(st->st_mode))
	{
		// st->st_mode = S_IFDIR | 0755;
		// st->st_nlink = 2; // Why "two" hardlinks instead of "one"? The answer is here: http://unix.stackexchange.com/a/101536
		printf("Inode number %d\n", st->st_ino);
		printf("is directory: %d\n", S_ISDIR(st->st_mode));
	}
	else if (S_ISREG(st->st_mode))
	{

		printf("File NOT directory\n");
	}
	





	
	
	





	return returnStatus;

}

static int do_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{


	cout << "\n\n\nUpdating Online Content------------------------" << endl;
	translateListOfFiles();
	while(wait == 0){

	}
	cout << "Finished Waiting" <<endl;
	wait = 0;
	if(listOfFiles.size() > contentMap.size()){
		translateDHTEntry();
		while(wait == 0){

		}
		cout << "Finished Waiting" <<endl;
		wait = 0;
	}
	cout << "Content updated to latest ODHT Table---------------------\n\n\n\n\n\n" << endl;








	cout << "\n\n\n\n\n------------ Synchro Table ------------\n\n";
	cout << "Synching to Files================================\n";
	if(listOfFiles.size()!= 0)
	{
		cout << "Current Size of Files: " << listOfFiles.size() << endl;
		int count = 0;
		for(auto i = listOfFiles.begin(); i != listOfFiles.end(); ++i)
		{
			cout << *i << " --index: "<< count << "\n";
			count ++;
		}
		cout << endl;
	}
	cout << "Finished Synching ================================\n\n";

	
	cout << "Synching to Written Content================================\n\n";
	if(contentMap.size()!= 0)
	{
		cout << "Current Size of Content: " << contentMap.size() << endl;
		cout << "PRINTING MAP CONTENT\n";
		for(auto& x: contentMap){
			cout << x.first << " : " << x.second << endl;
		}
	}
	cout << "Finished Synching ================================\n\n";
	

	cout << "Synching New Files=================================\n\n";

	std::set_difference(listOfFiles.begin(), listOfFiles.end(), PlistOfFiles.begin(), PlistOfFiles.end(),
						std::inserter(newFiles, newFiles.end()));

	if(newFiles.size()!= 0)
	{
		for(auto i = newFiles.begin(); i != newFiles.end(); ++i)
		{
			cout << *i << ", ";
		}
		cout << endl;
	}

	cout << "Finished New Files=================================\n\n\n\n";






	







	int retstat = 0;
	order++;
	printf("---------------------------------------------------------------do_readdir: %i\n", order);
	



	//Root dir stuff
	filler(buffer, ".", NULL, 0);
	filler(buffer, "..", NULL, 0);

	std::set<std::string>::iterator it = listOfFiles.begin();
	//Add extra entires
	if(listOfFiles.size()>0){
		if(strcmp(path,"/") == 0){
			while(it != listOfFiles.end()){
				//Add name of every OPDHT file
				filler(buffer, (char*)(*it).c_str(), NULL, 0);
			}
		}
	}
	return 0;







		
	DIR *dp;
	struct dirent *de;
		
		
	
	dp = (DIR *) (uintptr_t) fi->fh;
		

		

	// Every directory contains at least two entries: . and ..  If my
	// first call to the system readdir() returns NULL I've got an
	// error; near as I can tell, that's the only condition under
	// which I can get an error from readdir()

	printf("Before--------------------------\n");
	de = readdir(dp);
	printf("After--------------------------\n");

	if(de == 0)
	{
		return retstat; 
	}


	do 
	{
		// log_msg("calling filler with name %s\n", de->d_name);
		if (filler(buffer, de->d_name, NULL, 0) != 0) 
		{
			// log_msg("    ERROR bb_readdir filler:  buffer full");
			perror("error in readdir");
			return -ENOMEM;
		}
    } 
	while ((de = readdir(dp)) != NULL);
    



	







	return retstat;
}

/*
*/

static int do_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	order++;
	printf("-----------------------------------------------------------------------------doread: %i\n", order);
	int retstat = 0;

	const bool isINFILES = listOfFiles.find(path) != listOfFiles.end();
	if(isINFILES){

		std::string key = path;
		cout << "Reading: " << contentMap.find(key)->second << endl;
		
		memcpy(buffer, (char*)contentMap.find(key)->second.c_str() + offset,size);
		return strlen((char*)contentMap.find(key)->second.c_str()) - offset;
	}else{
		return -1;
	}



	retstat = pread(fi->fh, buffer, size, offset);

	if(retstat < 0)
	{
		perror("error in do_read ");
		return -errno;
	}

	return retstat;
	
}


static int do_mkdir(const char *path, mode_t mode)
{
	order++;
	printf("--------------------------------------do mkdir: %i\n", order);
	int returnStatus = -1;
	char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);


	mtx.lock();
	std::string found = path;
	cout << "TURNED PATH INTO A STIRNG " << found <<endl;
	
	cout << "... UPLOADING TO OPENDHT\n";
	wait = 0;
	node.put("LIST_OF_DIR", path,
	[](bool success){
		std::cout << "Put Path Finished with " << (success ? " Success." : " Failure.") << endl;
		wait = 1;
	});
	while(wait == 0){

	}
	wait = 0;
	
	mtx.unlock();
	return 0;

	/* Might not need if made in getattr
	returnStatus = mkdir(fpath, mode);

	if (returnStatus < 0)
	{
		perror("problem in mkdir: %s");
		return -errno;
	}
	return returnStatus;
	*/
}


/** Remove a file */
static int do_unlink(const char *path)
{
	order++;
	printf("-----------------------------------do unlink: %i\n", order);
	int returnstatus = 0;
    char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);
	returnstatus = unlink(fpath);
	if(returnstatus < 0){
		perror("Problem in do_unlink");
		return -errno;
	}

    return returnstatus;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
static int do_symlink(const char *path, const char *link)
{
	order++;
	printf("--------------------------------dosymlink: %i\n", order);
	int returnstatus = 0;
    char flink[PATH_MAX];
	strncpy(flink, mountpoint.path, PATH_MAX);
	strncat(flink, link, PATH_MAX);

	returnstatus = symlink(path, flink);
	if(returnstatus < 0){
		perror("Error in Symlink");
		return -errno;
	}

    return returnstatus;
}


static int do_rename(const char *path, const char *newpath)
{
	order++;
	printf("----------------------------------------rename: %i\n", order);

    int returnstatus = 0;
    char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	

    char fnewpath[PATH_MAX];
	strncpy(fnewpath, mountpoint.path, PATH_MAX);
	strncat(fpath, newpath, PATH_MAX);

	returnstatus = rename(fpath, fnewpath);
    if(returnstatus < 0){
		perror("Error in rename");
		return -errno;
	}
   

    return returnstatus;
}


/** Remove a directory */
static int do_rmdir(const char *path)
{
	order++;
	printf("----------------------------------------rmdir: %i\n", order);
    int returnstatus = 0;
    char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);
	returnstatus =  rmdir(fpath);
	if(returnstatus < 0){
		perror("Problem in do_unlink");
		return -errno;
	}

    return returnstatus;
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
static int do_utime(const char *path, struct utimbuf *ubuf)
{
	order++;
	printf("---------------utime: %i\n", order);
    
    int returnstatus = 0;
    char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
    
    returnstatus = utime(fpath, ubuf);
	if(returnstatus < 0){
		perror("Error in utime");
		return -errno;
	}


    return returnstatus;
}


static int do_mknod(const char *path, mode_t mode, dev_t rdev)
{
	order++;
	printf("----------------------------------------------domaknod: %i\n", order);

	int retstat;
    char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);
    
    
    // On Linux this could just be 'mknod(path, mode, dev)' but this
    // tries to be be more portable by honoring the quote in the Linux
    // mknod man page stating the only portable use of mknod() is to
    // make a fifo, but saying it should never actually be used for
    // that.
	
	//If everything is going to be downloaded don't need to make the nod
	return 0;

    if (S_ISREG(mode)) 
	{
		retstat = open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode);
		if (retstat >= 0)
		{
			retstat = close(retstat);
		}

		if (retstat < 0)
		{
			perror("Something went wrong in do_mknod S_ISREG: \n");
			retstat = -errno;
		}
	} 
	else
	{
		if (S_ISFIFO(mode))
		{
			retstat =mkfifo(fpath, mode);
			if (retstat < 0)
			{
				perror("Something went wrong in do_mknod S_ISFIFO: \n");
				retstat = -errno;
			}
		}
		else
		{
			retstat = mknod(fpath, mode, rdev);

				perror("Something went wrong in do_mknod ELSE: \n");
				retstat = -errno;
		}
	}

	return retstat;
}

static int do_write(const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *fi)
{
	order++;
	printf("--------------------------------------------------------------------------------------------dowrite: %i\n", order);

	/*
	if(inLocalFiles){

	}*/


	mtx.lock();
	std::string found = path;
	cout << "TURNED PATH INTO A STIRNG " << found <<endl;
	if(found.find(".swp") != std::string::npos){
		cout << "FOUND SWAP FILE IGNORING IT!" <<endl;
	}else{
		cout << "... UPLOADING TO OPENDHT\n";
		wait = 0;
		node.put("LIST_OF_FILES", path,
		[](bool success){
			std::cout << "Put Path Finished with " << (success ? " Success." : " Failure.") << endl;
			wait = 1;
		});
		while(wait == 0){

		}
		wait = 0;
		node.put(path, buffer,
		[](bool success){
			std::cout << "Put Content Finished with " << (success ? " Success." : " Failure.") << endl;
			wait = 1;
		});
		while(wait == 0){

		}
		wait = 0;
	}
	mtx.unlock();
	return size;



	char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);
	

	int retstat = 0;

	retstat = pwrite(fi->fh, buffer, size, offset);

	cout << "fpath also caoncatenated and put in node \n" ;


	if(retstat < 0)
	{
		perror("error in do_write ");
		return -errno;
	}

	

	// translateDHTEntry(path);
	//cout << "translateDHTEntry: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n" << dataRetrieved << endl;
 	// for (int i = 0; i < dataRetrieved.size(); ++i)
	// {

	return retstat;
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
static int do_access(const char *path, int mask)
{
	order++;
	cout << "-----------------in do_access: " << order << endl;
    int retstat = 0;
    char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);
	
    
    retstat = access(fpath, mask);
    
    if (retstat < 0){
		perror("Error in access");
		return -errno;
	}
	
    
    return retstat;
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
static int do_statfs(const char *path, struct statvfs *statv)
{
	order++;
	cout << "-------------in do_statfs: "   << order << endl;
    int retstat = 0;
	char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);
    
    
    // get stats for underlying filesystem
	retstat = statvfs(fpath, statv);
	if(retstat < 0){
		perror("Error in do statfs");
		return -errno;
	}
    
    return retstat;
}



/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
// this is a no-op in BBFS.  It just logs the call and returns success
int do_flush(const char *path, struct fuse_file_info *fi)
{
	order++;
	cout << "-------------in flush: "   << order << endl;
    return 0;
}




/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
static int do_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
   order++;
    cout << "-------------in fsync: "  << order << endl;
    // some unix-like systems (notably freebsd) don't have a datasync call
#ifdef HAVE_FDATASYNC
    if (datasync){
		int returnstatus = 0;
		returnstatus= fdatasync(fi->fh)
		if(returnstatus < 0){
			perror("Error in first part of fsync");
			return returnstatus;
		}
		return returnstatus;
	}
    else
#endif	
	int returnstatus =0;
	returnstatus = fsync(fi->fh);
	if(returnstatus < 0){
		perror("Error in second part of fsync");
		return returnstatus;
	}

	return returnstatus;
}


/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ??? >>> I need to implement this...
static int do_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
    int retstat = 0;
    order++;
    cout <<" -------------------------------------------FSYNCDIR"   << order << endl;
    
    return retstat;
}

/** Change the size of a file */
static int do_truncate(const char *path, off_t newsize)
{
	order++;
	cout << "------------------------------In Truncate " << order << endl;
    char fpath[PATH_MAX];
	strncpy(fpath, mountpoint.path, PATH_MAX);
	strncat(fpath, path, PATH_MAX);
	printf("Full absolute path created: %s\n", fpath);
	int returnstatus = 0;
	returnstatus = truncate(fpath, newsize);
	if(returnstatus < 0){
		perror("Error in do_truncate");
		return -errno;
	}

    return returnstatus;
}



static struct hello_fuse_operations:fuse_operations 
{
	hello_fuse_operations()
	{
	getattr = do_getattr; //1
	//.readlink = do_readlink; read a symbolic link
	readdir = do_readdir;
	getdir = NULL; //Deprecated
	mknod = do_mknod; //Creates non directory, non sym link nodes
	/*opendir = do_opendir;
	open = do_open;*/
	read = do_read;
	mkdir = do_mkdir; //makes a directory node
	/*
	unlink = do_unlink; //removes a file
	*/

	//symlink = do_symlink; //Who cares about symbolic links
    rename = do_rename; //rename a file or directory

	//link = do_link; Hard link meh who cares
  	//chmod = do_chmod; Change the permission bits, doesn't matter for our project
  	//chown = do_chown; change the owner or group of a file
  	
	  /*
	truncate = do_truncate; //change size of file
	utime = do_utime;  //change access/modification time
	rmdir = do_rmdir; //removes a directory
	*/

	write = do_write;
	/*
	release = do_release;
	statfs = do_statfs;
	flush = do_flush;
	fsync = do_fsync;

	access= do_access;

	fsyncdir = do_fsyncdir;*/
	}
} operations;




int main(int argc, char *argv[])
{

	int returnStatus;
	char *testpath;

	//No Root Users, Security issues arise!
	if ((getuid() == 0) || (geteuid() == 0))
	{
		fprintf(stderr, "Running FUSE file systems as root opens unnacceptable security holes\n");
		return 1;
	}

	//Returns absolute path
	mountpoint.path = realpath("rootdir", NULL);
	printf("MOUNTPATH GOT: %s\n", mountpoint.path);
	

	printf("---------This is the realpath should be absolute: %s\n", mountpoint.path);
	

	mountpoint.dir = (refreshfs_dirp*)malloc(sizeof(struct refreshfs_dirp));
	if (mountpoint.dir == NULL)
	{ //Not enough memory
		return -ENOMEM;
	}
	
	mountpoint.dir->offset = 0;
	mountpoint.dir->entry = NULL;




	std::cout << "starting DHT stuff" << std::endl; 

	//
    // Launch a dht node on a new thread, using a
    // generated RSA key pair, and listen on port 4222.
	//This is my node
    node.run(4222, dht::crypto::generateIdentity(), true);

    // Join the network through any running node,
    // here using a known bootstrap node.
    node.bootstrap("bootstrap.jami.net", "4222");

    // std::cout << "P Data" << std::endl;
    // node.put("fpath", "buffer");

    // std::cout << "-----------------" << std::endl;


    // wait for dht threads to end




	returnStatus = fuse_main(argc, argv, &operations, NULL);

	closedir(mountpoint.dir->dp);
	free(mountpoint.path);

	node.join();


	return returnStatus;
}
