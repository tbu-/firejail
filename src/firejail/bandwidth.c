#include "firejail.h"


void bandwidth_name(const char *name, const char *command, const char *dev, int down, int up) {
	if (!name || strlen(name) == 0) {
		fprintf(stderr, "Error: invalid sandbox name\n");
		exit(1);
	}
	pid_t pid;
	if (name2pid(name, &pid)) {
		fprintf(stderr, "Error: cannot find sandbox %s\n", name);
		exit(1);
	}

	bandwidth_pid(pid, command, dev, down, up);
}

void bandwidth_pid(pid_t pid, const char *command, const char *dev, int down, int up) {
	//************************
	// verify sandbox
	//************************
	char *comm = pid_proc_comm(pid);
	if (!comm) {
		fprintf(stderr, "Error: cannot find sandbox\n");
		exit(1);
	}

	// remove \n and check for firejail sandbox
	char *ptr = strchr(comm, '\n');
	if (ptr)
		*ptr = '\0';
	if (strcmp(comm, "firejail") != 0) {
		fprintf(stderr, "Error: cannot find sandbox\n");
		exit(1);
	}
	free(comm);
	
	// check network namespace
	char *cmd = pid_proc_cmdline(pid);
	if (!cmd || strstr(cmd, "--net") == NULL) {
		fprintf(stderr, "Error: the sandbox doesn't use a new network namespace\n");
		exit(1);
	}
	free(cmd);
	
	//************************
	// join the network namespace
	//************************
	pid_t child;
	if (find_child(pid, &child) == -1) {
		fprintf(stderr, "Error: cannot join the network namespace\n");
	}
	if (join_namespace(child, "net")) {
		fprintf(stderr, "Error: cannot join the network namespace\n");
		exit(1);
	}

	//************************
	// build command
	//************************
	char *devname = NULL;
	if (dev) {
		if (asprintf(&devname, "%s-%d", dev, pid) == -1)
			errExit("asprintf");
		// check device in namespace
		if (if_nametoindex(devname) == 0) {
			fprintf(stderr, "Error: cannot find network device %s\n", devname);
			exit(1);
		}
	}
	
	if (devname) {
		if (strcmp(command, "set") == 0) {
			if (asprintf(&cmd, "%s/lib/firejail/fshaper.sh --%s %s %d %d",
				PREFIX, command, devname, down, up) == -1)
				errExit("asprintf");
		}
		else {
			if (asprintf(&cmd, "%s/lib/firejail/fshaper.sh --%s %s", 
				PREFIX, command, devname) == -1)
				errExit("asprintf");
		}
	}
	else {
		if (asprintf(&cmd, "%s/lib/firejail/fshaper.sh --%s", PREFIX, command) == -1)
			errExit("asprintf");
	}

	//************************
	// build command
	//************************
	// elevate privileges
	uid_t uid = getuid();
	gid_t gid = getgid();
	if (setreuid(0, 0))
		errExit("setreuid");
	if (setregid(0, 0))
		errExit("setregid");

	char *arg[4];
	arg[0] = "/bin/bash";
	arg[1] = "-c";
	arg[2] = cmd;
	arg[3] = NULL;
	execvp("/bin/bash", arg);
	
	// it will never get here
	exit(0);		
}
