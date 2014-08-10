/*
 * Copyright (C) 2014 netblue30 (netblue30@yahoo.com)
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
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <errno.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include "firejail.h"

void join_namespace(pid_t pid, char *type) {
	char *path;
	if (asprintf(&path, "/proc/%u/ns/%s", pid, type) == -1)
		errExit("asprintf");
	
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: cannot open /proc/%u/ns/%s.\n", pid, type);
		exit(1);
	}

	if (syscall(__NR_setns, fd, 0) < 0) {
		fprintf(stderr, "Error: cannot join namespace.\n");
		exit(1);
	}

	close(fd);
	free(path);
}


#define BUFLEN 4096
// find the first child for this parent; return 1 if error
static int find_child(pid_t parent, pid_t *child) {
	*child = 0;	// use it to flag a found child

	DIR *dir;
	if (!(dir = opendir("/proc"))) {
		// sleep 2 seconds and try again
		sleep(2);
		if (!(dir = opendir("/proc"))) {
			fprintf(stderr, "Error: cannot open /proc directory\n");
			exit(1);
		}
	}
	
	struct dirent *entry;
	char *end;
	while (*child == 0 && (entry = readdir(dir))) {
		pid_t pid = strtol(entry->d_name, &end, 10);
		if (end == entry->d_name || *end)
			continue;
		if (pid == parent)
			continue;

		// open stat file
		char *file;
		if (asprintf(&file, "/proc/%u/status", pid) == -1) {
			perror("asprintf");
			exit(1);
		}
		FILE *fp = fopen(file, "r");
		if (!fp) {
			free(file);
			continue;
		}

		// look for firejail executable name
		char buf[BUFLEN];
		while (fgets(buf, BUFLEN - 1, fp)) {
			if (strncmp(buf, "PPid:", 5) == 0) {
				char *ptr = buf + 5;
				while (*ptr != '\0' && (*ptr == ' ' || *ptr == '\t')) {
					ptr++;
				}
				if (*ptr == '\0') {
					fprintf(stderr, "Error: cannot read /proc file\n");
					exit(1);
				}
				if (parent == atoi(ptr))
					*child = pid;
				break;	// stop reading the file
			}
		}
		fclose(fp);
		free(file);
	}
	closedir(dir);
	
	return (*child)? 0:1;	// 0 = found, 1 = not found
}

// return 1 if error
static int name2pid(const char *name, pid_t *pid) {
	pid_t parent = getpid();
	
	DIR *dir;
	if (!(dir = opendir("/proc"))) {
		// sleep 2 seconds and try again
		sleep(2);
		if (!(dir = opendir("/proc"))) {
			fprintf(stderr, "Error: cannot open /proc directory\n");
			exit(1);
		}
	}
	
	struct dirent *entry;
	char *end;
	while ((entry = readdir(dir))) {
		pid_t newpid = strtol(entry->d_name, &end, 10);
		if (end == entry->d_name || *end)
			continue;
		if (newpid == parent)
			continue;

		// check if this is a firejail executable
		char *comm = pid_proc_comm(newpid);
		if (comm) {
			// remove \n
			char *ptr = strchr(comm, '\n');
			if (ptr)
				*ptr = '\0';
			if (strcmp(comm, "firejail")) {
				free(comm);
				continue;
			}
			free(comm);
		}
		
		char *cmd = pid_proc_cmdline(newpid);
		if (cmd) {
			// mark the end of the name
			char *ptr = strstr(cmd, "--name=");
			char *start = ptr;
			if (!ptr) {
				free(cmd);
				continue;
			}
			while (*ptr != ' ' && *ptr != '\t' && *ptr != '\0')
				ptr++;
			*ptr = '\0';
			int rv = strcmp(start + 7, name);
			if (rv == 0) {
				free(cmd);
				*pid = newpid;
				closedir(dir);
				return 0;
			}
			free(cmd);
		}
	}
	closedir(dir);
	return 1;
}

void shut_name(const char *name) {
	if (!name || strlen(name) == 0) {
		fprintf(stderr, "Error: invalid sandbox name\n");
		exit(1);
	}
	
	pid_t pid;
	if (name2pid(name, &pid)) {
		fprintf(stderr, "Error: cannot find sandbox %s\n", name);
		exit(1);
	}

	shut(pid);
}

void shut(pid_t pid) {
	// if the pid is that of a firejail  process, use the pid of a child process inside the sandbox
	char *comm = pid_proc_comm(pid);
	if (comm) {
		// remove \n
		char *ptr = strchr(comm, '\n');
		if (ptr)
			*ptr = '\0';
		if (strcmp(comm, "firejail") == 0) {
			pid_t child;
			if (find_child(pid, &child) == 0) {
				pid = child;
				printf("Switching to pid %u, the first child process inside the sandbox\n", (unsigned) pid);
			}
		}
		free(comm);
	}

	// check privileges for non-root users
	uid_t uid = getuid();
	if (uid != 0) {
		struct stat s;
		char *dir;
		if (asprintf(&dir, "/proc/%u/ns", pid) == -1)
			errExit("asprintf");
		if (stat(dir, &s) < 0)
			errExit("stat");
		if (s.st_uid != uid) {
			fprintf(stderr, "Error: permission is denied to shutdown a sandbox created by a different user.\n");
			exit(1);
		}
	}
	
	printf("Sending SIGTERM to %u\n", pid);
	kill(pid, SIGTERM);
	sleep(2);
	
	// if the process is still running, terminate it using SIGKILL
	// try to open stat file
	char *file;
	if (asprintf(&file, "/proc/%u/status", pid) == -1) {
		perror("asprintf");
		exit(1);
	}
	FILE *fp = fopen(file, "r");
	if (!fp)
		return;
	fclose(fp);
	printf("Sending SIGKILL to %u\n", pid);
	kill(pid, SIGKILL);
}

void join_name(const char *name, const char *homedir) {
	if (!name || strlen(name) == 0) {
		fprintf(stderr, "Error: invalid sandbox name\n");
		exit(1);
	}
	pid_t pid;
	if (name2pid(name, &pid)) {
		fprintf(stderr, "Error: cannot find sandbox %s\n", name);
		exit(1);
	}

	join(pid, homedir);
}

void join(pid_t pid, const char *homedir) {
	// if the pid is that of a firejail  process, use the pid of a child process inside the sandbox
	char *comm = pid_proc_comm(pid);
	if (comm) {
		// remove \n
		char *ptr = strchr(comm, '\n');
		if (ptr)
			*ptr = '\0';
		if (strcmp(comm, "firejail") == 0) {
			pid_t child;
			if (find_child(pid, &child) == 0) {
				pid = child;
				printf("Switching to pid %u, the first child process inside the sandbox\n", (unsigned) pid);
			}
		}
		free(comm);
	}

	// check privileges for non-root users
	uid_t uid = getuid();
	if (uid != 0) {
		struct stat s;
		char *dir;
		if (asprintf(&dir, "/proc/%u/ns", pid) == -1)
			errExit("asprintf");
		if (stat(dir, &s) < 0)
			errExit("stat");
		if (s.st_uid != uid) {
			fprintf(stderr, "Error: permission is denied to join a sandbox created by a different user.\n");
			exit(1);
		}
	}

	// join namespaces
	join_namespace(pid, "ipc");
	join_namespace(pid, "net");
	join_namespace(pid, "pid");
	join_namespace(pid, "uts");
	join_namespace(pid, "mnt");
	

	pid_t child = fork();
	if (child < 0)
		errExit("fork");
	if (child == 0) {
		// chroot into /proc/PID/root directory
		char *rootdir;
		if (asprintf(&rootdir, "/proc/%d/root", pid) == -1)
			errExit("asprintf");
			
		int rv = chroot(rootdir); // this will fail for processes in sandboxes not started with --chroot option
		if (rv == 0)
			printf("changing root to %s\n", rootdir);
		
		prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0); // kill the child in case the parent died
		if (chdir("/") < 0)
			errExit("chdir");
		if (homedir) {
			struct stat s;
			if (stat(homedir, &s) == 0) {
				if (chdir(homedir) < 0)
					errExit("chdir");
			}
		}
		
		// fix qt 4.8
		if (setenv("QT_X11_NO_MITSHM", "1", 1) < 0)
			errExit("setenv");
		if (setenv("container", "firejail", 1) < 0) // LXC sets container=lxc,
			errExit("setenv");
		// drop privileges
		drop_privs();

		// set prompt color to green
		//export PS1='\[\e[1;32m\][\u@\h \W]\$\[\e[0m\] '
		if (setenv("PROMPT_COMMAND", "export PS1=\"\\[\\e[1;32m\\][\\u@\\h \\W]\\$\\[\\e[0m\\] \"", 1) < 0)
			errExit("setenv");

		// replace the process with a regular bash session
		execlp("/bin/bash", "/bin/bash", NULL);
		// it will never get here!!!
	}

	// wait for the child to finish
	waitpid(child, NULL, 0);
	exit(0);
}



