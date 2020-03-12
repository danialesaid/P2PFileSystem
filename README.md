# P2PFileSystem
A Peer to Peer file system built on top of FUSE (File System in User Space ). 
P2PFS is an inmemory file system meaning that all files and directories created 
will not exist once that filesystem is unmounted.


# Installing FUSE
In order to get the filesystem working several conditions need to be met.

Ubuntu: FUSE is not Windows supported by default. The FUSE Kernel Module is not installed and even if you use 
the Windows Subsystem for Linux, you'll run to this problem.

While FUSE comes pre installed with most versions of Linux we still need extra supportting software to support building our file system.
With a fresh install of Ubuntu, download and install the following.


## FUSE Requirements

On the Ubuntu Terminal.

* sudo apt-get update
* sudo apt-get install fuse
* sudo apt-get install libfuse-dev

Download the fuse library of functions
https://github.com/libfuse/libfuse

In order to test if libfuse is working well
Will need the following

meson and ninja needed to build libfuse 
* sudo apt-get install meson

For Meson to work we need to build essentials
* sudo apt-get upgrade
* sudo apt-get install build essential
* sudo apt-get install pkg-config

Need Python3 to run the tests to see if FUSE is working
* sudo apt-get install python3.7
* sudo apt-get install python3-pip
* pip3 install --user pytest

Then just follow instructions on libfuse github readMe
until you see passing tests. Make sure a majority of the tests pass.

Now your OS is ready to support a FUSE filesystem. 

## OpenDHT Requirements


* sudo apt-get --upgrade

* sudo apt-get install build-essential

* sudo apt install cmake


* sudo apt install autoconf

Install OpenDHT dependencies

 * sudo apt install libncurses5-dev libreadline-dev nettle-dev libgnutls28-dev libargon2-0-dev libmsgpack-dev librestbed-dev libjsoncpp-dev

clone the repo

* git clone https://github.com/savoirfairelinux/opendht.git

build and install

cd opendht
mkdir build && cd build
cmake -DOPENDHT_PYTHON=OFF -DCMAKE_INSTALL_PREFIX=/usr ..

Workaround for asio from issues thread in OpenDHT repository

wget https://github.com/aberaud/asio/archive/b2b7a1c166390459e1c169c8ae9ef3234b361e3f.tar.gz \
&& tar -xvf b2b7a1c166390459e1c169c8ae9ef3234b361e3f.tar.gz && cd asio-b2b7a1c166390459e1c169c8ae9ef3234b361e3f/asio \
&& ./autogen.sh && ./configure --prefix=/usr --without-boost --disable-examples --disable-tests  \
&& sudo make install

cd ..
cd ..

In the build folder
make -j4
sudo make install


## Building our P2P File System

Now that you installed Fuse and OpenDHT you are ready to make our filesystem.

* Open P2PFileSystem Folder
* Open src Folder
* mkdir rootdir 
* mkdir Mountingpoint
* make clean
* make
* ./refreshFS -d -s Mountingpoint

This will make the file system run in single threaded mode and in devmode. 
Once it starts you will see the fuse file system operations on screen. Open a new 
tab on the terminal and navigate to the mounting point. You can now create and
edit files in a P2P filesystem.

## Testing and Running

Currently we are debugging the program using GDB. In order
to run the program using GDB in the src folder hit the command

* gdb refreshFS

Once in the gdb environment you can run the command
* file refreshFS

This will allow you to set break points using the command 'b [line number]'.

To skip the break points once entering gdb just enter

 * run -s -d Mountdir
 * -s: Runs the file system as a single thread program
 * -d: Allows for debugging options
 * Mountdir: is the name of the folder that will be tourned to our mounting point.

ctrl c and q exits the program.




To unmount use the instruction

* fusermount -u [mounting point]



## About the File System

The base of our P2P file system started out from. LSYSFS, a less simple yet stupid file system.
LSYSFS is a in memory(all files disapear on dismount) file system that is as simple as it gets, it reads only one directory level down. With 
LSYSFS as a base we looked at how each function worked. What does each function need? What does it return?
Having a basic understanding of how the file system worked we could then build on top of it by looking at 
more feature complete file systems and how they handle routines that every file system needs. 


### Pre Req Knowledge

Before you dive in there are some things you must know.

* Everything is a file, even am a file
In Unix everything is a file. Yes that means a directory is a file too. 


* Paths
You need to know what's the difference between a relative path and an absolute path. 

https://stackoverflow.com/questions/46121000/what-is-an-absolute-pathname-vs-a-relative-pathname

Whenver you have to deal with an object(file or directory) in our filesystem, fuse will scan everything in a directory and return
the path of files and directories. Say you want to write some file, Fuse will send a write request
to our file system with the full path "/mountdir/filename". If your file is in the root of your file system it will look like "/filename".


* The difference between a Root Directory and Mounting Point

The difference between the mounting point and root directory is something that caused me to waste several weeks on a bug. Because I didn't have background information on this topic I was trying to solve a problem that was a deeper issue at hand. You may have not noticed it browsing other file systems but there should always be two parameters to give fuse. A mounting point and a root directory. Why is this? This is something you should explore on your own. But know, if understand this a lot of possible questions could be answered.


* Inodes
Files and their contents are not stored together but are found together with their index position. The relationship will help us find the file and its contents. Inodes are a data structure in unix type file systemsthat describe a file or directory. Inodes store the attributes and disk block locations
of the objects data. File-system object attributes may include metadata (times of last change, access, modification), as well as owner and permission data. Directories are lists of names assigned to inodes. A directory contains an entry for itself, its parent, and each of its children. As you have seen you can't create a directory or file of the same name because a list is maintaining each file and its unique name.

https://en.wikipedia.org/wiki/Inode


Below are the functions we explored and what they do.

* getattr: 

This function gets the file attributes. Basically it tells the OS if this is a file or directory. It's perhaps the first step that hapens in fuse, if you don't make an init function.
"Also, it tells about the owner, group and permission bits of the object. This function is a vital one that without it we can’t have a functional filesystem"

* readir:

This function takes usually takes place after the getattr function. Before a moving into a directory fuse first needs to read the contents of the directory. The elements passed into this function are important as these are the structures required to read a directory. The function of the event readdir provides the operating system with the list of files/directories that reside in a given directory. A buffer, a fuse filler of dir t, an offset and structure of fuse file info.

The buffer is where we will fill the list of available files and directories that reside in the path given to us by fuse. Something else provided by fuse is the filler. With the filler function given to us by fuse we can use it to fill the buffer. In the readir function the first thing we will do is fill it with two directories "." and ".." explanation here (http://unix.stackexchange.com/a/101536).

Something to keep in mind is we need to get information about files in the directory which is where fuse_file_info comes into play. A file handle or fh in our case is a File Structure that may contain a file descrptor usually stating whether a file is open or not. https://unix.stackexchange.com/questions/146113/file-handles-and-filenames. We get a pointer to this file structure because fh is a file handle id. We call this pointer dp. The id fh now in dp is used as an index with the syscall readdir(dp) we can then return what the file handle id represents. A structure of type dirent contaning 

    struct dirent {

               ino_t          d_ino;       /* Inode number */
               
               off_t          d_off;       /* Not an offset; see below */
               
               unsigned short d_reclen;    /* Length of this record */
               
               unsigned char  d_type;      /* Type of file; not supported
               
                                              by all filesystem types */
                                              
                                              
               char           d_name[256]; /* Null-terminated filename */
               
    };

http://man7.org/linux/man-pages/man3/readdir.3.html

We will call this pointer that we got from reddir() de short for directory entry. The first check is to see if the directory entry is null. If it's null something is wrong because every directory, as stated before should have "." and ".." as its first directory entries. Once we verify that the first de is not null we can loop and continue to use readdir(dp) for all of the entries until we reach the end which is null. In the loop we call filler, filling it with the de->dname and at the same time check if there are no errors.

With that in mind lets refresh what we did in this function. We made a directory pointer dp and set it to fh(another fuse function sets up fh), then we use that fh(file handle id) that's in dp to read the directory which returns the directory entry structure above. We place the name of the directory entry in a filler and loop until we reach the end of dp. In short this function puts names of null terminated files in a filler. 

You can see how this could be useful for a a file function like LS. In the current version of our file system and our existing fuse functions the order of operations when making an ls call in a terminal is 

* getatr

* opendir

* getatr

* readir

The reasoning behind this is because getattr is always being called first, next the fh is setup by opendir, then getattr is called again, then readdir uses the fh to read the contents in the directory.







## Resources

IBM Developer: https://developer.ibm.com/articles/l-fuse/

* If you were looking for FUSE resources before hand this was perhaps the first link that popped up.
Don't listen to anything about the FUSE set up, that's an older version that caused us issues. Instead
just pay attention the function descriptions. That's probably the most useful information you'll get.

