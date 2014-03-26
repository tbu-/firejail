#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <glob.h>
#include "firejail.h"


//***********************************************
// atexit
//***********************************************
static char tmpdir[PATH_MAX];

static void bye(void) {
	assert(tmpdir);
	if (!arg_command)
		printf("\nbye...\n");

	char cmd[strlen(tmpdir) + 20];
	sprintf(cmd, "rm -fr %s", tmpdir);
	if (system(cmd) < 0)
		errExit("system");
}

void set_exit(pid_t child) {
	sprintf(tmpdir, "/tmp/firejail.dir.%u", child);
	if (atexit(bye))
		errExit("atexit");
}


//***********************************************
// chroot filesystem
//***********************************************
static void mnt_tmp(void) {
	if (arg_debug)
		printf("Mounting new /tmp and /var/tmp directories\n");
	if (mount("tmpfs", "/var/tmp", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV | MS_STRICTATIME | MS_REC,  "mode=755,gid=0") < 0)
		errExit("/var");

	if (mount("tmpfs", "/tmp", "tmpfs", MS_NOSUID | MS_NOEXEC | MS_NODEV | MS_STRICTATIME | MS_REC,  "mode=777,gid=0") < 0)
		errExit("/tmp");
}

static void disable_file(const char *fname, const char *emptydir, const char *emptyfile) {
	assert(fname);
	assert(emptydir);
	assert(emptyfile);
	
	struct stat s;
	if (stat(fname, &s) == 0) {
		if (S_ISDIR(s.st_mode)) {
			if (mount(emptydir, fname, "none", MS_BIND, "mode=400,gid=0") < 0)
				errExit(fname);
		}
		else {
			if (mount(emptyfile, fname, "none", MS_BIND, "mode=400,gid=0") < 0)
				errExit(fname);
		}
		if (arg_debug)
			printf("Disabling %s\n", fname);
	}
}

static void globbing(const char *fname, const char *emptydir, const char *emptyfile) {
	assert(fname);
	assert(emptydir);
	assert(emptyfile);

	// filename globbing: expand * macro and continue processing for every single file
	if (strchr(fname, '*')) {
		glob_t globbuf;
		globbuf.gl_offs = 0;
		glob(fname, GLOB_DOOFFS, NULL, &globbuf);
		int i;
		for (i = 0; i < globbuf.gl_pathc; i++) {
			assert(globbuf.gl_pathv[i]);
			disable_file(globbuf.gl_pathv[i], emptydir, emptyfile);
		}
	}
	else
		disable_file(fname, emptydir, emptyfile);
}

static void expand_path(const char *path, const char *fname, const char *emptydir, const char *emptyfile) {
	assert(path);
	assert(fname);
	assert(emptydir);
	assert(emptyfile);
	char newname[strlen(path) + strlen(fname) + 1];
	sprintf(newname, "%s%s", path, fname);
	
	globbing(newname, emptydir, emptyfile);
}

// blacklist files or directoies by mounting empty files on top of them
void mnt_blacklist(char **blacklist, const char *homedir, const char *childstr) {
	char *tmpdir;
	char *emptydir;
	char *emptyfile;
	
	if (arg_debug)
		printf("Creating /tmp/firejail.dir.%s directory\n", childstr);

	// create tmp directory
	if (asprintf(&tmpdir, "/tmp/firejail.dir.%s", childstr) == -1)
		errExit("asprintf");
	mkdir(tmpdir, S_IRWXU);
	uid_t u = getuid();
	gid_t g = getgid();
	if (chown(tmpdir, u, g) < 0)
		errExit("chown");

	// create read-only root directory
	if (asprintf(&emptydir, "/tmp/firejail.dir.%s/firejail.ro.dir.%s", childstr, childstr) == -1)
		errExit("asprintf");
	mkdir(emptydir, S_IRWXU);
	if (chown(emptydir, 0, 0) < 0)
		errExit("chown");

	// create read-only root file
	if (asprintf(&emptyfile, "/tmp/firejail.dir.%s/firejail.ro.file.%s", childstr, childstr) == -1)
		errExit("asprintf");
	FILE *fp = fopen(emptyfile, "w");
	if (!fp)
		errExit("fopen");
	fclose(fp);
	if (chown(emptyfile, 0, 0) < 0)
		errExit("chown");
	if (chmod(emptyfile, S_IRUSR) < 0)
		errExit("chown");

	int i = 0;
	while (blacklist[i]) {
		// process newtmp macro
		if (strncmp(blacklist[i], "newdir", 6) == 0) {
			if (strcmp(blacklist[i], "newdir /tmp") == 0)
				mnt_tmp();
			else
				fprintf(stderr, "Warning: %s not implemented yet\n", blacklist[i]);
			i++;
			continue;
		}

		// process blacklist command
		if (strncmp(blacklist[i], "blacklist", 9) != 0) {
			fprintf(stderr, "Error: invalid profile line %s\n", blacklist[i]);
			i++;
			continue;
		}
				
		
		char *ptr = blacklist[i] + 10;
		// replace home macro in blacklist array
		char *new_name = NULL;
		if (strncmp(ptr, "${HOME}", 7) == 0) {
			if (asprintf(&new_name, "%s%s", homedir, ptr + 7) == -1)
				errExit("asprintf");
			ptr = new_name;
		}
		
		// expand path macro - look for the file in /bin, /usr/bin, /sbin and  /usr/sbin directories
		if (strncmp(ptr, "${PATH}", 7) == 0) {
			expand_path("/bin", ptr + 7, emptydir, emptyfile);
			expand_path("/sbin", ptr + 7, emptydir, emptyfile);
			expand_path("/usr/bin", ptr + 7, emptydir, emptyfile);
			expand_path("/usr/sbin", ptr + 7, emptydir, emptyfile);
		}
		else
			globbing(ptr, emptydir, emptyfile);
		if (new_name)
			free(new_name);
		i++;
	}
	
	free(tmpdir);
	free(emptydir);
	free(emptyfile);
}


// remount a directory read-only
void mnt_rdonly(const char *dir) {
	assert(dir);
	// check directory exists
	struct stat s;
	int rv = stat(dir, &s);
	if (rv == 0) {
		// mount --bind /bin /bin
		if (mount(dir, dir, NULL, MS_BIND|MS_REC, NULL) < 0)
			errExit(dir);
		// mount --bind -o remount,ro /bin
		if (mount(NULL, dir, NULL, MS_BIND|MS_REMOUNT|MS_RDONLY|MS_REC, NULL) < 0)
			errExit(dir);
	}
}


// mount /proc and /sys directories
void mnt_proc_sys(void) {
	if (arg_debug)
		printf("Remounting /proc and /proc/sys filesystems\n");
	if (mount("proc", "/proc", "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV | MS_REC, NULL) < 0)
		errExit("/proc");

	if (mount("/proc/sys", "/proc/sys", NULL, MS_BIND | MS_REC, NULL) < 0)
		errExit("/proc/sys");

	if (mount(NULL, "/proc/sys", NULL, MS_BIND | MS_REMOUNT | MS_RDONLY | MS_REC, NULL) < 0)
		errExit("/proc/sys");

	//	if (mount("sysfs", "/sys", "sysfs", MS_RDONLY|MS_NOSUID|MS_NOEXEC|MS_NODEV|MS_REC, NULL) < 0)
	//		errExit("/sys");
}

// build a basic read-only filesystem
void mnt_basic_fs(void) {
	if (arg_debug)
		printf("Mounting read-only /bin, /sbin, /lib, /lib64, /usr, /boot, /etc, and /opt\n");
	mnt_rdonly("/bin");
	mnt_rdonly("/sbin");
	mnt_rdonly("/lib");
	mnt_rdonly("/lib64");
	mnt_rdonly("/usr");
	mnt_rdonly("/boot");
	mnt_rdonly("/etc");
	mnt_rdonly("/opt");
	if (arg_debug)
		printf("Mounting a new /root directory\n");
	if (mount("tmpfs", "/root", "tmpfs", MS_STRICTATIME | MS_REC,  "mode=700,gid=0") < 0)
		errExit("/root");
//	if (system("cp /etc/skel/.bashrc /root/. 2>&1") == -1)
//		errExit("system");

	// check /run directory exists
	struct stat s;
	int rv = stat("/run", &s);
	if (rv == 0) {
		if (arg_debug)
			printf("Mounting a new /run directory\n");
		if (mount("tmpfs", "/run", "tmpfs", MS_NOSUID | MS_NODEV | MS_STRICTATIME | MS_REC,  "mode=777,gid=0") < 0)
			errExit("/run");
		if (system("mkdir /run/shm") == -1)
			errExit("system");
		if (chown("/run/shm", 0, 0))
			errExit("chown");
		if (chmod("/run/shm",
			S_IRUSR | S_IWUSR | S_IXUSR |
			S_IRGRP | S_IWGRP | S_IXGRP |
			S_IROTH | S_IWOTH | S_IXOTH))
			errExit("chmod");
	}
	else {
		if (arg_debug)
			printf("Mounting a new /var/run directory\n");
		if (mount("tmpfs", "/var/run", "tmpfs", MS_NOSUID | MS_NODEV | MS_STRICTATIME | MS_REC,  "mode=777,gid=0") < 0)
			errExit("/var/run");
	}
	
}

void mnt_home(const char *homedir) {
	if (arg_debug)
		printf("Mounting a new /home directory\n");  
	if (mount("tmpfs", "/home", "tmpfs", MS_NOSUID | MS_NODEV | MS_STRICTATIME | MS_REC,  "mode=755,gid=0") < 0)
		errExit("/home");
	mkdir(homedir, S_IRWXU|S_IRGRP|S_IXGRP);
	uid_t u = getuid();
	gid_t g = getgid();
	if (chown(homedir, u, g) == -1)
		errExit("chown");
	
	// copy skel files
	char *cmd;
	if (asprintf(&cmd, "cp -r /etc/skel/.bashrc %s/.", homedir) == -1)
		errExit("asprintf");
	if (system(cmd) == -1)
		errExit("system");
	free(cmd);
	
	if (asprintf(&cmd, "%s/.bashrc", homedir) == -1)
		errExit("asprintf");
	if (chown(cmd, u, g) == -1)
		errExit("chown");
	free(cmd);
}