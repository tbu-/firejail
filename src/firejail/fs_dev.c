/*
 * Copyright (C) 2014, 2015 netblue30 (netblue30@yahoo.com)
 *
 * This file is part of firejail project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "firejail.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <glob.h>
#include <dirent.h>
#include <fcntl.h>
#include <pwd.h>

static void create_dev(const char *path, const char *cmd) {
	int rv = system(cmd);
	if (rv == -1)
		goto errexit;
	if (chown(path, 0, 0) < 0)
		goto errexit;
	return;
	
errexit:
	fprintf(stderr, "Error: cannot create %s device\n", path);
	exit(1);
}

static void create_link(const char *oldpath, const char *newpath) {
	if (symlink(oldpath, newpath) == -1)
		goto errexit;
	if (chown(newpath, 0, 0) < 0)
		goto errexit;
	return;

errexit:
	fprintf(stderr, "Error: cannot create %s device\n", newpath);
	exit(1);
}

void fs_private_dev(void){
	// install a new /dev directory
	if (arg_debug)
		printf("Mounting tmpfs on /dev\n");
	if (mount("tmpfs", "/dev", "tmpfs", MS_NOSUID | MS_STRICTATIME | MS_REC,  "mode=777,gid=0") < 0)
		errExit("mounting /dev");
	
	// create /dev/shm
	if (arg_debug)
		printf("Create /dev/shm directory\n");
	mkdir("/dev/shm", S_IRWXU | S_IRWXG | S_IRWXO);
	if (chown("/dev/shm", 0, 0) < 0)
		errExit("chown");
	if (chmod("/dev/shm", S_IRWXU | S_IRWXG | S_IRWXO) < 0)
		errExit("chmod");

	// create devices
	create_dev("/dev/zero", "mknod -m 666 /dev/zero c 1 5");
	create_dev("/dev/null", "mknod -m 666 /dev/null c 1 3");
	create_dev("/dev/full", "mknod -m 666 /dev/full c 1 7");
	create_dev("/dev/random", "mknod -m 666 /dev/random c 1 8");
	create_dev("/dev/urandom", "mknod -m 666 /dev/urandom c 1 9");
	create_dev("/dev/tty", "mknod -m 666 /dev/tty c 5 0");
#if 0
	create_dev("/dev/tty0", "mknod -m 666 /dev/tty0 c 4 0");
	create_dev("/dev/console", "mknod -m 622 /dev/console c 5 1");

	// pseudo-terminal
	mkdir("/dev/pts", 0755);
	if (chown("/dev/pts", 0, 0) < 0)
		errExit("chown");
	if (chmod("/dev/pts", 0755) < 0)
		errExit("chmod");
	create_dev("/dev/pts/ptmx", "mknod -m 666 /dev/pts/ptmx c 5 2");
	create_link("/dev/pts/ptmx", "/dev/ptmx");
//system("mount -vt devpts -o gid=4,mode=620 none /dev/pts");	

	// stdin, stdout, stderr
	create_link("/proc/self/fd", "/dev/fd");
	create_link("/proc/self/fd/0", "/dev/stdin");
	create_link("/proc/self/fd/1", "/dev/stdout");
	create_link("/proc/self/fd/2", "/dev/stderr");
#endif
}


void fs_dev_shm(void) {
	uid_t uid = getuid(); // set a new shm only if we started as root
	if (uid)
		return;

	if (is_dir("/dev/shm")) {
		if (arg_debug)
			printf("Mounting tmpfs on /dev/shm\n");
		if (mount("tmpfs", "/dev/shm", "tmpfs", MS_NOSUID | MS_STRICTATIME | MS_REC,  "mode=777,gid=0") < 0)
			errExit("mounting /dev/shm");
	}
	else {
		char *lnk = get_link("/dev/shm");
		if (lnk) {
			// convert a link such as "../shm" into "/shm"
			char *lnk2 = lnk;
			int cnt = 0;
			while (strncmp(lnk2, "../", 3) == 0) {
				cnt++;
				lnk2 = lnk2 + 3;
			}
			if (cnt != 0)
				lnk2 = lnk + (cnt - 1) * 3 + 2;

			if (!is_dir(lnk2)) {
				// create directory
				if (mkdir(lnk2, S_IRWXU|S_IRWXG|S_IRWXO))
					errExit("mkdir");
				if (chown(lnk2, 0, 0))
					errExit("chown");
				if (chmod(lnk2, S_IRWXU|S_IRWXG|S_IRWXO))
					errExit("chmod");
			}
			if (arg_debug)
				printf("Mounting tmpfs on %s on behalf of /dev/shm\n", lnk2);
			if (mount("tmpfs", lnk2, "tmpfs", MS_NOSUID | MS_STRICTATIME | MS_REC,  "mode=777,gid=0") < 0)
				errExit("mounting /var/tmp");
			free(lnk);
		}
		else {
			fprintf(stderr, "Warning: /dev/shm not mounted\n");
			dbg_test_dir("/dev/shm");
		}
			
	}
}
