/*
 * Copyright (C) 2014, 2015 Firejail Authors
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
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <sched.h>
#ifndef CLONE_NEWUSER
#define CLONE_NEWUSER	0x10000000
#endif

static void set_caps(void) {
	if (arg_caps_drop_all)
		caps_drop_all();
	else if (arg_caps_drop)
		caps_drop_list(arg_caps_list);
	else if (arg_caps_keep)
		caps_keep_list(arg_caps_list);
	else if (arg_caps_default_filter)
		caps_default_filter();
}

void save_nogroups(void) {
	if (arg_nogroups == 0)
		return;

	char *fname;
	if (asprintf(&fname, "%s/groups", MNT_DIR) == -1)
		errExit("asprintf");
	FILE *fp = fopen(fname, "w");
	if (fp) {
		fprintf(fp, "\n");
		fclose(fp);
		if (chown(fname, 0, 0) < 0)
			errExit("chown");
	}
	else {
		fprintf(stderr, "Error: cannot save nogroups state\n");
		free(fname);
		exit(1);
	}
	
	free(fname);
}

static void sandbox_if_up(Bridge *br) {
	assert(br);
	if (!br->configured)
		return;
		
	char *dev = br->devsandbox;
	net_if_up(dev);

	if (br->arg_ip_none == 1);	// do nothing
	else if (br->arg_ip_none == 0 && br->macvlan == 0) {
		if (br->ipsandbox == br->ip) {
			fprintf(stderr, "Error: %d.%d.%d.%d is interface %s address.\n", PRINT_IP(br->ipsandbox), br->dev);
			exit(1);
		}
		
		// just assign the address
		assert(br->ipsandbox);
		if (arg_debug)
			printf("Configuring %d.%d.%d.%d address on interface %s\n", PRINT_IP(br->ipsandbox), dev);
		net_if_ip(dev, br->ipsandbox, br->mask);
		net_if_up(dev);
	}
	else if (br->arg_ip_none == 0 && br->macvlan == 1) {
		// reassign the macvlan address
		if (br->ipsandbox == 0)
			br->ipsandbox = arp_assign(dev, br->ip, br->mask);
		else {
			if (br->ipsandbox == br->ip) {
				fprintf(stderr, "Error: %d.%d.%d.%d is interface %s address.\n", PRINT_IP(br->ipsandbox), br->dev);
				exit(1);
			}
			
			uint32_t rv = arp_check(dev, br->ipsandbox, br->ip);
			if (rv) {
				fprintf(stderr, "Error: the address %d.%d.%d.%d is already in use.\n", PRINT_IP(br->ipsandbox));
				exit(1);
			}
		}
			
		if (arg_debug)
			printf("Configuring %d.%d.%d.%d address on interface %s\n", PRINT_IP(br->ipsandbox), dev);
		net_if_ip(dev, br->ipsandbox, br->mask);
		net_if_up(dev);
	}
}

static void chk_chroot(void) {
		// if we are starting firejail inside some other container technology, we don't care about this
	char *mycont = getenv("container");
	if (mycont)
		return;
	
	// check if this is a regular chroot
	struct stat s;
	if (stat("/", &s) == 0) {
		if (s.st_ino != 2)
			return;
	}
	
	fprintf(stderr, "Error: cannot mount filesystem as slave\n");
	exit(1);
}

int sandbox(void* sandbox_arg) {
	pid_t child_pid = getpid();
	if (arg_debug)
		printf("Initializing child process\n");	

 	// close each end of the unused pipes
 	close(parent_to_child_fds[1]);
 	close(child_to_parent_fds[0]);
 
 	// wait for parent to do base setup
 	wait_for_other(parent_to_child_fds[0]);

	if (arg_debug && child_pid == 1)
		printf("PID namespace installed\n");

	//****************************
	// set hostname
	//****************************
	if (cfg.hostname) {
		if (sethostname(cfg.hostname, strlen(cfg.hostname)) < 0)
			errExit("sethostname");
	}

	//****************************
	// mount namespace
	//****************************
	// mount events are not forwarded between the host the sandbox
	if (mount(NULL, "/", NULL, MS_SLAVE | MS_REC, NULL) < 0) {
		chk_chroot();
	}
	
	//****************************
	// netfilter
	//****************************
	if (arg_netfilter && any_bridge_configured()) { // assuming by default the client filter
		netfilter(arg_netfilter_file);
	}

	//****************************
	// trace pre-install
	//****************************
	if (arg_trace)
		fs_trace_preload();

	//****************************
	// configure filesystem
	//****************************
#ifdef HAVE_CHROOT		
	if (cfg.chrootdir) {
		fs_chroot(cfg.chrootdir);
		// force caps and seccomp if not started as root
		if (getuid() != 0) {
			// force default seccomp inside the chroot, no keep or drop list
			// the list build on top of the default drop list is kept intact
			arg_seccomp = 1;
			if (arg_seccomp_list_drop) {
				free(arg_seccomp_list_drop);
				arg_seccomp_list_drop = NULL;
			}
			if (arg_seccomp_list_keep) {
				free(arg_seccomp_list_keep);
				arg_seccomp_list_keep = NULL;
			}
			
			// disable all capabilities
			if (arg_caps_default_filter || arg_caps_list)
				fprintf(stderr, "Warning: all capabilities disabled for a regular user during chroot\n");
			arg_caps_drop_all = 1;
			
			// drop all supplementary groups; /etc/group file inside chroot
			// is controlled by a regular usr
			arg_nogroups = 1;
			printf("Dropping all Linux capabilities and enforcing default seccomp filter\n");
		}
						
		//****************************
		// trace pre-install, this time inside chroot
		//****************************
		if (arg_trace)
			fs_trace_preload();
	}
	else 
#endif		
	if (arg_overlay)
		fs_overlayfs();
	else
		fs_basic_fs();
	

	//****************************
	// set hostname in /etc/hostname
	//****************************
	if (cfg.hostname) {
		fs_hostname(cfg.hostname);
	}
	
	//****************************
	// apply the profile file
	//****************************
	if (cfg.profile)
		fs_blacklist(cfg.homedir);
	
	//****************************
	// private mode
	//****************************
	if (arg_private) {
		if (cfg.home_private)	// --private=
			fs_private_homedir();
		else if (cfg.home_private_keep) // --private.keep=
			fs_private_home_list();
		else // --private
			fs_private();
	}
	
	if (arg_private_dev)
		fs_private_dev();
	
	//****************************
	// install trace
	//****************************
	if (arg_trace)
		fs_trace();
		
	//****************************
	// update /proc, /dev, /boot directorymy
	//****************************
	fs_proc_sys_dev_boot();
	
	//****************************
	// networking
	//****************************
	if (arg_nonetwork) {
		net_if_up("lo");
	}
	else if (any_bridge_configured()) {
		// configure lo and eth0...eth3
		net_if_up("lo");
		sandbox_if_up(&cfg.bridge0);
		sandbox_if_up(&cfg.bridge1);
		sandbox_if_up(&cfg.bridge2);
		sandbox_if_up(&cfg.bridge3);
		
		// add a default route
		if (cfg.defaultgw) {
			// set the default route
			if (net_add_route(0, 0, cfg.defaultgw))
				fprintf(stderr, "Warning: cannot configure default route\n");
		}
			
		if (arg_debug)
			printf("Network namespace enabled\n");
	}
	net_ifprint();
	
	// if any dns server is configured, it is time to set it now
	fs_resolvconf();
	

	//****************************
	// start executable
	//****************************
	prctl(PR_SET_PDEATHSIG, SIGKILL, 0, 0, 0); // kill the child in case the parent died
	int cwd = 0;
	if (cfg.cwd) {
		if (chdir(cfg.cwd) == 0)
			cwd = 1;
	}
	
	if (!cwd) {
		if (chdir("/") < 0)
			errExit("chdir");
		if (cfg.homedir) {
			struct stat s;
			if (stat(cfg.homedir, &s) == 0) {
				if (chdir(cfg.homedir) < 0)
					errExit("chdir");
			}
		}
	}
	
	// set environment
	// fix qt 4.8
	if (setenv("QT_X11_NO_MITSHM", "1", 1) < 0)
		errExit("setenv");
	if (setenv("container", "firejail", 1) < 0) // LXC sets container=lxc,
		errExit("setenv");
	if (arg_zsh && setenv("SHELL", "/usr/bin/zsh", 1) < 0)
		errExit("setenv");
	if (arg_csh && setenv("SHELL", "/bin/csh", 1) < 0)
		errExit("setenv");
	if (cfg.shell && setenv("SHELL", cfg.shell, 1) < 0)
		errExit("setenv");
	// set prompt color to green
	//export PS1='\[\e[1;32m\][\u@\h \W]\$\[\e[0m\] '
	if (setenv("PROMPT_COMMAND", "export PS1=\"\\[\\e[1;32m\\][\\u@\\h \\W]\\$\\[\\e[0m\\] \"", 1) < 0)
		errExit("setenv");
		

	// set capabilities
	if (!arg_noroot)
		set_caps();

	// set rlimits
	set_rlimits();

	// set seccomp
#ifdef HAVE_SECCOMP
	// if a keep list is available, disregard the drop list
	if (arg_seccomp == 1) {
		if (arg_seccomp_list_keep)
			seccomp_filter_keep(); // this will also save the fmyilter to MNT_DIR/seccomp file
		else
			seccomp_filter_drop(); // this will also save the filter to MNT_DIR/seccomp file
	}
#endif

	// set cpu affinity
	if (cfg.cpus) {
		save_cpu(); // save cpu affinity mask to MNT_DIR/cpu file
		set_cpu_affinity();
	}
	
	// save cgroup in MNT_DIR/cgroup file
	if (cfg.cgroup)
		save_cgroup();
		
	// drop privileges
	save_nogroups();
	drop_privs(arg_nogroups);

	//****************************************
	// create new user namespace now that privileged child setup is complete
	//****************************************
	if (arg_noroot)
		unshare(CLONE_NEWUSER);
 	
 	// notify parent that new user namespace has been created so a proper
 	// UID/GID map can be setup
 	notify_other(child_to_parent_fds[1]);
 	close(child_to_parent_fds[1]);
 
 	// wait for parent to finish setting up a proper UID/GID map
 	wait_for_other(parent_to_child_fds[0]);
 	close(parent_to_child_fds[0]);

	// somehow, the new user namespace resets capabilities;
	// we need to do them again
	if (arg_noroot)
		set_caps();


	//****************************************
	// start the program without using a shell
	//****************************************
	if (arg_shell_none) {
		if (arg_debug) {
			int i;
			for (i = cfg.original_program_index; i < cfg.original_argc; i++) {
				if (cfg.original_argv[i] == NULL)
					break;
				printf("execvp argument %d: %s\n", i - cfg.original_program_index, cfg.original_argv[i]);
			}
		}

		if (!arg_command)
			printf("Child process initialized\n");
		execvp(cfg.original_argv[cfg.original_program_index], &cfg.original_argv[cfg.original_program_index + 1]);
	}
	//****************************************
	// start the program using a shell
	//****************************************
	else {
		// choose the shell requested by the user, or use bash as default
		char *sh;
		if (cfg.shell)
	 		sh = cfg.shell;
		else if (arg_zsh)
			sh = "/usr/bin/zsh";
		else if (arg_csh)
			sh = "/bin/csh";
		else
			sh = "/bin/bash";
			
		char *arg[5];
		int index = 0;
		arg[index++] = sh;
		arg[index++] = "-c";
		assert(cfg.command_line);
		if (arg_debug)
			printf("Starting %s\n", cfg.command_line);
		if (arg_doubledash) 
			arg[index++] = "--";
		arg[index++] = cfg.command_line;
		arg[index] = NULL;
		assert(index < 5);
		
		if (arg_debug) {
			char *msg;
			if (asprintf(&msg, "sandbox %d, execvp into %s", sandbox_pid, cfg.command_line) == -1)
				errExit("asprintf");
			logmsg(msg);
			free(msg);
		}
		
		if (arg_debug) {
			int i;
			for (i = 0; i < 5; i++) {
				if (arg[i] == NULL)
					break;
				printf("execvp argument %d: %s\n", i, arg[i]);
			}
		}
		
		if (!arg_command)
			printf("Child process initialized\n");
		execvp(sh, arg);
	}
	

	perror("execvp");
	return 0;
}
