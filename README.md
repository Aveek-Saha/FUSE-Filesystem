# FUSE Filesystem
A basic file system written in C using FUSE


# About fuse
From the [official repository](https://github.com/libfuse/libfuse)
>  FUSE (Filesystem in Userspace) is an interface for userspace programs to export a filesystem to the Linux kernel. The FUSE project consists of two components: the fuse kernel module and the libfuse userspace library. libfuse provides the reference implementation for communicating with the FUSE kernel module.

Basically Fuse allows us to call our own functions instead of using the default kernel functions when a system call is used. That is incoming requests from the kernel are passed to the main program using callbacks. Where we can define our own functions to handle them.


# Installing FUSE
For Ubuntu
```
$ sudo apt-get install libfuse-dev
```


# Using the Filesystem

Clone this repository
```
$ git clone https://github.com/Aveek-Saha/FUSE-Filesystem.git FS
```

cd into the directory and create a mount point
```
$ cd FS
$ mkdir mountpoint
```
Complile and run FS.c
```
$ gcc FS.c -o FS `pkg-config fuse --cflags --libs`
$ ./ FS - f path/ to/ mountpoint
```
Change your current working directory to ```mountpoint``` and use the file system.


# Operations

The following operations are implimented -
- Create and Remove a directory.
- Create, Read and write to a file.
- Delete an existing file.
- Appending to and truncating a file.
- Access, modified and status change time updates.
- Open and close a file.

# Team
This project was a team effort by

| Name | GitHub Profile |
|:---:|:---:|
|  Arvind Srinivasan | [arvindsrinivasan](https://github.com/arvindsrinivasan)  |
|  Aprameya Bharadwaj |  [aprameyabharadwaj](https://github.com/aprameyabharadwaj) |
|  Anish Kasi | [anishkasi](https://github.com/anishkasi)  |
