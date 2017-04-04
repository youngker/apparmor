/*
 *	Copyright (C) 2002-2006 Novell/SUSE
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2 of the
 *	License.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/types.h>
#include <dirent.h>
#include <linux/unistd.h>
#include <string.h>
#include <sys/syscall.h>

int main(int argc, char *argv[])
{
	int fd;
#if defined(SYS_getdents64)
	struct dirent64 dir;
#else
	struct dirent dir;
#endif

	if (argc != 2){
		fprintf(stderr, "usage: %s dir\n",
			argv[0]);
		return 1;
	}

	fd = open(argv[1], O_RDONLY, 0);
	if (fd == -1){
		printf("FAIL - open  %s\n", strerror(errno));
		return 1;
	}

	/*
	if (fchdir(fd) == -1){
		printf("FAIL - fchdir  %s\n", strerror(errno));
		return 1;
	}
	*/

	/* getdents isn't exported by glibc, so must use syscall() */
#if defined(SYS_getdents64)
	if (syscall(SYS_getdents64, fd, &dir, sizeof(struct dirent64)) == -1){
#else
	if (syscall(SYS_getdents, fd, &dir, sizeof(struct dirent)) == -1){
#endif
		printf("FAIL - getdents  %s\n", strerror(errno));
		return 1;
	}

	printf("PASS\n");

	return 0;
}
