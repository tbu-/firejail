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

static char *client_filter = 
"*filter\n"
":INPUT DROP [0:0]\n"
":FORWARD DROP [0:0]\n"
":OUTPUT ACCEPT [0:0]\n"
"-A INPUT -i lo -j ACCEPT\n"
"# echo replay is handled by -m state RELEATED/ESTABLISHED below\n"
"#-A INPUT -p icmp --icmp-type echo-reply -j ACCEPT\n"
"-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n"
"-A INPUT -p icmp --icmp-type destination-unreachable -j ACCEPT\n"
"-A INPUT -p icmp --icmp-type time-exceeded -j ACCEPT\n"
"-A INPUT -p icmp --icmp-type echo-request -j ACCEPT \n"
"COMMIT\n";

void netfilter(const char *fname) {
	// default filter
	char *filter = client_filter;

	// custom filter
	int allocated = 0;
	if (fname) {
		// buffer the filter
		struct stat s;
		if (stat(fname, &s) == -1) {
			fprintf(stderr, "Error: cannot find network filter file\n");
			exit(1);
		}
				
		filter = malloc(s.st_size + 1);	// + '\0'
		memset(filter, 0, s.st_size + 1);
		if (!filter)
			errExit("malloc");
		
		FILE *fp = fopen(fname, "r");
		if (!fp) {
			fprintf(stderr, "Error: cannot open network filter file\n");
			exit(1);
		}

		size_t sz = fread(filter, 1, s.st_size, fp);
		if (sz != s.st_size) {
			fprintf(stderr, "Error: cannot read network filter file\n");
			exit(1);
		}
		fclose(fp);
		allocated = 1;
	}
		
	// mount a tempfs on top of /tmp directory
	if (mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID | MS_STRICTATIME | MS_REC,  "mode=755,gid=0") < 0)
		errExit("mounting /tmp");

	// create the filter file
	FILE *fp = fopen("/tmp/netfilter", "w");
	if (!fp) {
		fprintf(stderr, "Error: cannot open /tmp/netfilter file\n");
		exit(1);
	}
	fprintf(fp, "%s\n", filter);
	fclose(fp);

	// find iptables command
	struct stat s;
	char *iptables = NULL;
	if (stat("/sbin/iptables", &s) == 0)
		iptables = "/sbin/iptables";
	else if (stat("/usr/sbin/iptables", &s) == 0)
		iptables = "/usr/sbin/iptables";
	else if (stat("/bin/iptables", &s) == 0)
		iptables = "/bin/iptables";
	else if (stat("/usr/bin/iptables", &s) == 0)
		iptables = "/usr/bin/iptables";
	if (iptables == NULL) {
		fprintf(stderr, "Error: iptables command not found\n");
		goto doexit;
	}		

	// push filter
	int rv;
	if (arg_debug)
		printf("Installing network filter:\n%s\n", filter);

	char *cmd;
	if (asprintf(&cmd, "%s-restore < /tmp/netfilter", iptables) == -1)
		errExit("asprintf");
	rv = system(cmd);
	if (rv == -1) {
		fprintf(stderr, "Error: failed to configure network filter.\n");
		exit(1);
	}
	free(cmd);
	
	if (arg_debug) {
		if (asprintf(&cmd, "%s -vL", iptables) == -1)
			errExit("asprintf");
		rv = system(cmd);
		free(cmd);
	}
	
doexit:	
	// unmount /tmp
	umount("/tmp");

	if (allocated)
		free(filter);
}


