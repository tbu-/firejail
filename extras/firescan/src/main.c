#include "firescan.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <linux/if_packet.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <net/route.h>
#include <linux/if_bridge.h>

#define _GNU_SOURCE
#include <sys/utsname.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>
#include <limits.h>
#include <sys/file.h>
#include <sys/prctl.h>
#include <signal.h>
#include <time.h>



int arg_debug = 1;
#define STACK_SIZE (1024 * 1024)
static char child_stack[STACK_SIZE];		// space for child's stack
int parent_to_child_fds[2];
int child_to_parent_fds[2];

char *dev = "eth0";
char *devsandbox;
uint32_t ifip;
uint32_t ifmask;
uint8_t mac[6];
int sock;




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

	// mount events are not forwarded between the host the sandbox
//	int rv = mount(NULL, "/", NULL, MS_SLAVE | MS_REC, NULL);
//	(void) rv; // disregard return value, we might already be in a chroot

	// bring up ethernet device
//	net_if_up("lo");
	net_if_up(devsandbox);

system("/sbin/ifconfig");

	// notify parent that new user namespace has been created so a proper
 	// UID/GID map can be setup
 	notify_other(child_to_parent_fds[1]);
 	close(child_to_parent_fds[1]);
 
 	// wait for parent to finish setting up a proper UID/GID map
 	wait_for_other(parent_to_child_fds[0]);
 	close(parent_to_child_fds[0]);

	// open layer2 socket
	if ((sock = socket(PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0)
		errExit("socket");

//	drop_privs();	

	printf("Scanning interface %s, %d.%d.%d.%d/%d\n",
		devsandbox, PRINT_IP(ifip), mask2bits(ifmask));
	arp_sequential(sock, devsandbox, ifip, ifmask, mac);


	return 0;
}


int main(int argc, char **argv) {
	pid_t parent = getpid();
	
	// extract network data
	if (net_get_addr(dev, &ifip, &ifmask)) {
		fprintf(stderr, "Error: cannot find interface %s\n", dev);
		return 1;
	}
	net_get_mac(dev, mac);
	if (asprintf(&devsandbox, "%s-%d", dev, parent) == -1)
		errExit("asprintf");
	

	// create the parent-child communication pipes
 	if (pipe(parent_to_child_fds) < 0)
 		errExit("pipe");
 	if (pipe(child_to_parent_fds) < 0)
		errExit("pipe");

	// clone environment
	int flags = /*CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWUTS |*/ CLONE_NEWNET | SIGCHLD;

	pid_t child = clone(sandbox,
		child_stack + STACK_SIZE,
		flags,
		NULL);
	if (child == -1)
		errExit("clone");
	printf("Parent pid %u, child pid %u\n", parent, child);


	// attach a macvlan device to the sandbox
	net_create_macvlan(devsandbox, dev, child);
	
 	// close each end of the unused pipes
 	close(parent_to_child_fds[0]);
 	close(child_to_parent_fds[1]);
 
 	// notify child that base setup is complete
 	notify_other(parent_to_child_fds[1]);
 
 	// wait for child to create new user namespace with CLONE_NEWUSER
 	wait_for_other(child_to_parent_fds[0]);
 	close(child_to_parent_fds[0]);
	
	// over here goes user namespace configuration
	
 	// notify child that UID/GID mapping is complete
 	notify_other(parent_to_child_fds[1]);
 	close(parent_to_child_fds[1]);
	
	waitpid(child, NULL, 0);
	return 0;
}
