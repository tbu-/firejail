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
#include <errno.h>
#include <linux/filter.h>
#include <stddef.h>
#include <linux/capability.h>
#include <linux/audit.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

extern int capget(cap_user_header_t hdrp, cap_user_data_t datap);
extern int capset(cap_user_header_t hdrp, const cap_user_data_t datap);


typedef struct {
	char *name;
	int nr;
} CapsEntry;

static CapsEntry capslist[] = {
//
// code generated using tools/extract-caps
// updated manually based on kernel 3.18/include/linux/capability.h (added block suspend and audit_read
//
#ifdef CAP_CHOWN
	{"chown", CAP_CHOWN },
#endif
#ifdef CAP_DAC_OVERRIDE
	{"dac_override", CAP_DAC_OVERRIDE },
#endif
#ifdef CAP_DAC_READ_SEARCH
	{"dac_read_search", CAP_DAC_READ_SEARCH },
#endif
#ifdef CAP_FOWNER
	{"fowner", CAP_FOWNER },
#endif
#ifdef CAP_FSETID
	{"fsetid", CAP_FSETID },
#endif
#ifdef CAP_KILL
	{"kill", CAP_KILL },
#endif
#ifdef CAP_SETGID
	{"setgid", CAP_SETGID },
#endif
#ifdef CAP_SETUID
	{"setuid", CAP_SETUID },
#endif
#ifdef CAP_SETPCAP
	{"setpcap", CAP_SETPCAP },
#endif
#ifdef CAP_LINUX_IMMUTABLE
	{"linux_immutable", CAP_LINUX_IMMUTABLE },
#endif
#ifdef CAP_NET_BIND_SERVICE
	{"net_bind_service", CAP_NET_BIND_SERVICE },
#endif
#ifdef CAP_NET_BROADCAST
	{"net_broadcast", CAP_NET_BROADCAST },
#endif
#ifdef CAP_NET_ADMIN
	{"net_admin", CAP_NET_ADMIN },
#endif
#ifdef CAP_NET_RAW
	{"net_raw", CAP_NET_RAW },
#endif
#ifdef CAP_IPC_LOCK
	{"ipc_lock", CAP_IPC_LOCK },
#endif
#ifdef CAP_IPC_OWNER
	{"ipc_owner", CAP_IPC_OWNER },
#endif
#ifdef CAP_SYS_MODULE
	{"sys_module", CAP_SYS_MODULE },
#endif
#ifdef CAP_SYS_RAWIO
	{"sys_rawio", CAP_SYS_RAWIO },
#endif
#ifdef CAP_SYS_CHROOT
	{"sys_chroot", CAP_SYS_CHROOT },
#endif
#ifdef CAP_SYS_PTRACE
	{"sys_ptrace", CAP_SYS_PTRACE },
#endif
#ifdef CAP_SYS_PACCT
	{"sys_pacct", CAP_SYS_PACCT },
#endif
#ifdef CAP_SYS_ADMIN
	{"sys_admin", CAP_SYS_ADMIN },
#endif
#ifdef CAP_SYS_BOOT
	{"sys_boot", CAP_SYS_BOOT },
#endif
#ifdef CAP_SYS_NICE
	{"sys_nice", CAP_SYS_NICE },
#endif
#ifdef CAP_SYS_RESOURCE
	{"sys_resource", CAP_SYS_RESOURCE },
#endif
#ifdef CAP_SYS_TIME
	{"sys_time", CAP_SYS_TIME },
#endif
#ifdef CAP_SYS_TTY_CONFIG
	{"sys_tty_config", CAP_SYS_TTY_CONFIG },
#endif
#ifdef CAP_MKNOD
	{"mknod", CAP_MKNOD },
#endif
#ifdef CAP_LEASE
	{"lease", CAP_LEASE },
#endif
#ifdef CAP_AUDIT_WRITE
	{"audit_write", CAP_AUDIT_WRITE },
#endif
#ifdef CAP_AUDIT_CONTROL
	{"audit_control", CAP_AUDIT_CONTROL },
#endif
#ifdef CAP_SETFCAP
	{"setfcap", CAP_SETFCAP },
#endif
#ifdef CAP_MAC_OVERRIDE
	{"mac_override", CAP_MAC_OVERRIDE },
#endif
#ifdef CAP_MAC_ADMIN
	{"mac_admin", CAP_MAC_ADMIN },
#endif
#ifdef CAP_SYSLOG
	{"syslog", CAP_SYSLOG },
#endif
#ifdef CAP_WAKE_ALARM
	{"wake_alarm", CAP_WAKE_ALARM },
#endif
// not in Debian 7
#ifdef CAP_BLOCK_SUSPEND
	{"block_suspend", CAP_BLOCK_SUSPEND },
#else	
	{"block_suspend", 36 },
#endif
#ifdef CAP_AUDIT_READ
	{"audit_read", CAP_AUDIT_READ },
#else	
	{"audit_read", 37 },
#endif

//
// end of generated code
//
}; // end of capslist

const char *caps_find_nr(int nr) {
	int i;
	int elems = sizeof(capslist) / sizeof(capslist[0]);
	for (i = 0; i < elems; i++) {
		if (nr == capslist[i].nr)
			return capslist[i].name;
	}
	
	return "unknown";
}

// return -1 if error, or syscall number
static int caps_find_name(const char *name) {
	int i;
	int elems = sizeof(capslist) / sizeof(capslist[0]);
	for (i = 0; i < elems; i++) {
		if (strcmp(name, capslist[i].name) == 0)
			return capslist[i].nr;
	}
	
	return -1;
}

// return 1 if error, 0 if OK
int caps_check_list(const char *clist, void (*callback)(int)) {
	// don't allow empty lists
	if (clist == NULL || *clist == '\0') {
		fprintf(stderr, "Error: empty capabilities lists are not allowed\n");
		return -1;
	}

	// work on a copy of the string
	char *str = strdup(clist);
	if (!str)
		errExit("strdup");

	char *ptr = str;
	char *start = str;
	while (*ptr != '\0') {
		if (islower(*ptr) || isdigit(*ptr) || *ptr == '_')
			;
		else if (*ptr == ',') {
			*ptr = '\0';
			int nr = caps_find_name(start);
			if (nr == -1) {
				fprintf(stderr, "Error: capability %s not found\n", start);
				return -1;
			}
			else if (callback != NULL)
				callback(nr);
				
			start = ptr + 1;
		}
		ptr++;
	}
	if (*start != '\0') {
		int nr = caps_find_name(start);
		if (nr == -1) {
			fprintf(stderr, "Error: capability %s not found\n", start);
			return -1;
		}
		else if (callback != NULL)
			callback(nr);
	}

	free(str);	
	return 0;
}

void caps_print(void) {
	int i;
	int elems = sizeof(capslist) / sizeof(capslist[0]);
	for (i = 0; i < elems; i++) {
		printf("%d\t- %s\n", capslist[i].nr, capslist[i].name);
	}
}




// enabled by default
int caps_default_filter(void) {
	// drop capabilities
	if (prctl(PR_CAPBSET_DROP, CAP_SYS_MODULE, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_SYS_MODULE");
	else if (arg_debug)
		printf("Drop CAP_SYS_MODULE\n");

	if (prctl(PR_CAPBSET_DROP, CAP_SYS_RAWIO, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_SYS_RAWIO");
	else if (arg_debug)
		printf("Drop CAP_SYS_RAWIO\n");

	if (prctl(PR_CAPBSET_DROP, CAP_SYS_BOOT, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_SYS_BOOT");
	else if (arg_debug)
		printf("Drop CAP_SYS_BOOT\n");

	if (prctl(PR_CAPBSET_DROP, CAP_SYS_NICE, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_SYS_NICE");
	else if (arg_debug)
		printf("Drop CAP_SYS_NICE\n");

	if (prctl(PR_CAPBSET_DROP, CAP_SYS_TTY_CONFIG, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_SYS_TTY_CONFIG");
	else if (arg_debug)
		printf("Drop CAP_SYS_TTY_CONFIG\n");

	if (prctl(PR_CAPBSET_DROP, CAP_SYSLOG, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_SYSLOG");
	else if (arg_debug)
		printf("Drop CAP_SYSLOG\n");

	if (prctl(PR_CAPBSET_DROP, CAP_MKNOD, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_MKNOD");
	else if (arg_debug)
		printf("Drop CAP_MKNOD\n");

	if (prctl(PR_CAPBSET_DROP, CAP_SYS_ADMIN, 0, 0, 0) && arg_debug)
		fprintf(stderr, "Warning: cannot drop CAP_SYS_ADMIN");
	else if (arg_debug)
		printf("Drop CAP_SYS_ADMIN\n");

	return 0;
}

void caps_drop_all(void) {
	if (arg_debug)
		printf("Droping all capabilities\n");

	unsigned long cap;
	for (cap=0; cap <= 63; cap++) {
		int code = prctl(PR_CAPBSET_DROP, cap, 0, 0, 0);
		if (code == -1 && errno != EINVAL)
			errExit("PR_CAPBSET_DROP");
	}
}


void caps_set(uint64_t caps) {
	if (arg_debug)
		printf("Set caps filter %llx\n", (unsigned long long) caps);

	unsigned long i;
	uint64_t mask = 1LLU;
	for (i = 0; i < 64; i++, mask <<= 1) {
		if ((mask & caps) == 0) {
			int code = prctl(PR_CAPBSET_DROP, i, 0, 0, 0);
			if (code == -1 && errno != EINVAL)
				errExit("PR_CAPBSET_DROP");
		}
	}
}


static uint64_t filter;

static void caps_set_bit(int nr) {
	uint64_t mask = 1LLU << nr;
	filter |= mask;
}
static void caps_reset_bit(int nr) {
	uint64_t mask = 1LLU << nr;
	filter &= ~mask;
}

void caps_drop_list(const char *clist) {
	filter = 0;
	filter--;
	caps_check_list(clist, caps_reset_bit);
	caps_set(filter);
}

void caps_keep_list(const char *clist) {
	filter = 0;
	caps_check_list(clist, caps_set_bit);
	caps_set(filter);
}

#define MAXBUF 4098
static uint64_t extract_caps(int pid) {
	char *file;
	if (asprintf(&file, "/proc/%d/status", pid) == -1) {
		errExit("asprintf");
		exit(1);
	}

	FILE *fp = fopen(file, "r");
	if (!fp) {
		printf("Error: cannot open %s\n", file);
		free(file);
		exit(1);
	}
	
	char buf[MAXBUF];
	while (fgets(buf, MAXBUF, fp)) {
		if (strncmp(buf, "CapBnd:", 7) == 0) {
			char *ptr = buf + 8;
			unsigned long long val;
			sscanf(ptr, "%llx", &val);
			free(file);
			fclose(fp);
			return val;
		}
	}
	fclose(fp);
	free(file);
	printf("Error: cannot read caps configuration\n");
	exit(1);
}


void caps_print_filter_name(const char *name) {
	if (!name || strlen(name) == 0) {
		fprintf(stderr, "Error: invalid sandbox name\n");
		exit(1);
	}
	pid_t pid;
	if (name2pid(name, &pid)) {
		fprintf(stderr, "Error: cannot find sandbox %s\n", name);
		exit(1);
	}

	caps_print_filter(pid);
}

void caps_print_filter(pid_t pid) {
	// if the pid is that of a firejail  process, use the pid of the first child process
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
			printf("Error: permission denied.\n");
			exit(1);
		}
	}

	uint64_t caps = extract_caps(pid);
	drop_privs(1);

	int i;
	uint64_t mask;
	int elems = sizeof(capslist) / sizeof(capslist[0]);
	for (i = 0, mask = 1; i < elems; i++, mask <<= 1) {
		printf("%-18.18s  - %s\n", capslist[i].name, (mask & caps)? "enabled": "disabled");
	}

	exit(0);
}
