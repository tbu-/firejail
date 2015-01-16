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
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if_ether.h>			  //TCP/IP Protocol Suite for Linux
#include <net/if.h>
#include <netinet/in.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>
#include <unistd.h>
#include <string.h>
#include <linux/if_packet.h>

#include "firejail.h"

typedef struct arp_hdr_t {
	uint16_t htype;
	uint16_t ptype;
	uint8_t hlen;
	uint8_t plen;
	uint16_t opcode;
	uint8_t sender_mac[6];
	uint8_t sender_ip[4];
	uint8_t target_mac[6];
	uint8_t target_ip[4];
} ArpHdr;

// returns 0 if the address is not in use, -1 otherwise
int arp_check(const char *dev, uint32_t destaddr, uint32_t srcaddr) {
	if (arg_debug)
		printf("Trying %d.%d.%d.%d ...\n", PRINT_IP(destaddr));

	// find eth0 interface address
	int sock;
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		errExit("socket");

	srcaddr = htonl(srcaddr);
	destaddr = htonl(destaddr);

	// FInd eth0 interface MAC address
	struct ifreq ifr;
	memset(&ifr, 0, sizeof (ifr));
	snprintf(ifr.ifr_name, sizeof (ifr.ifr_name), "%s", dev);
	if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0)
		errExit("ioctl");
	close(sock);
	
	// configure layer2 socket address information
	struct sockaddr_ll addr;
	memset(&addr, 0, sizeof(addr));
	if ((addr.sll_ifindex = if_nametoindex(dev)) == 0)
		errExit("if_nametoindex");
	addr.sll_family = AF_PACKET;
	memcpy (addr.sll_addr, ifr.ifr_hwaddr.sa_data, 6);
	addr.sll_halen = htons(6);

	// build the arp packet header
	ArpHdr hdr;
	memset(&hdr, 0, sizeof(hdr));
	hdr.htype = htons(1);
	hdr.ptype = htons(ETH_P_IP);
	hdr.hlen = 6;
	hdr.plen = 4;
	hdr.opcode = htons(1); //ARPOP_REQUEST
	memcpy(hdr.sender_mac, ifr.ifr_hwaddr.sa_data, 6);
	memcpy(hdr.sender_ip, (uint8_t *)&srcaddr, 4);
	memcpy(hdr.target_ip, (uint8_t *)&destaddr, 4);

	// buiild ethernet frame
	uint8_t frame[ETH_FRAME_LEN]; // includes eht header, vlan, and crc
	memset(frame, 0, sizeof(frame));
	frame[0] = frame[1] = frame[2] = frame[3] = frame[4] = frame[5] = 0xff;
	memcpy(frame + 6, ifr.ifr_hwaddr.sa_data, 6);
	frame[12] = ETH_P_ARP / 256;
	frame[13] = ETH_P_ARP % 256;
	memcpy (frame + 14, &hdr, sizeof(hdr));

	// open layer2 socket
	if ((sock = socket(PF_PACKET, SOCK_RAW, htons (ETH_P_ALL))) < 0)
		errExit("socket");

	int len;
	if ((len = sendto (sock, frame, 14 + sizeof(ArpHdr), 0, (struct sockaddr *) &addr, sizeof (addr))) <= 0)
		errExit("send");
	fflush(0);
		
	// wait not more than one second for an answer
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(sock, &fds);
	int maxfd = sock;
	struct timeval ts;
	ts.tv_sec = 1; // 1 second wait time
	ts.tv_usec = 0;
	while (1) {
		int nready = select(maxfd + 1,  &fds, (fd_set *) 0, (fd_set *) 0, &ts);
		if (nready < 0)
			errExit("select");
		else if (nready == 0) { // timeout
			close(sock);
			return 0;
		}
		else {
			// read the incoming packet
			int len = recvfrom(sock, frame, ETH_FRAME_LEN, 0, NULL, NULL);
			if (len < 0) {
				perror("recvfrom");
				close(sock);
				return -1;
			}
			
			// parse the incomming packet
			if (len < 14 + sizeof(ArpHdr))
				continue;
			if (frame[12] != ETH_P_ARP / 256 || frame[13] != ETH_P_ARP % 256)
				continue;
			memcpy(&hdr, frame + 14, sizeof(ArpHdr));
			if (hdr.opcode == htons(1))
				continue;
			if (hdr.opcode == htons(2)) {
				// check my mac and my address
				if (memcmp(ifr.ifr_hwaddr.sa_data, hdr.target_mac, 6) != 0)
					continue;
				uint32_t ip;
				memcpy(&ip, hdr.target_ip, 4);
				if (ip != srcaddr) {
					continue;
				}					
				close(sock);
				return -1;
			}
		}
	}

	// it will never get here!
	close(sock);
	return -1;
}

// assign a random IP address and check it
uint32_t arp_random(const char *dev, uint32_t ifip, uint32_t ifmask) {
	assert(ifip);
	assert(ifmask);
	assert(dev);

	uint32_t range = ~ifmask + 1; // the number of potential addresses
	// this software is not supported for /31 networks
	if (range < 4)
		return 0; // the user will have to set the IP address manually
	range -= 2; // subtract the network address and the broadcast address
	if (arg_debug)
		printf("IP address range from %d.%d.%d.%d to %d.%d.%d.%d\n",
			PRINT_IP((ifip & ifmask) + 1), PRINT_IP((ifip & ifmask) + range));

	// try a random address
	uint32_t dest = (ifip & ifmask) + 1 + ((uint32_t) rand()) % range;
	while (dest == ifip)
		dest = (ifip & ifmask) + 1 + ((uint32_t) rand()) % range;
	uint32_t rv = arp_check(dev, dest, ifip);
	if (!rv)
		return dest;
	return 0;
}

// go sequentially trough all IP addresses and assign the first one not in use
uint32_t arp_sequential(const char *dev, uint32_t ifip, uint32_t ifmask) {
	assert(ifip);
	assert(ifmask);
	assert(dev);

	uint32_t range = ~ifmask + 1; // the number of potential addresses
	// this software is not supported for /31 networks
	if (range < 4)
		return 0; // the user will have to set the IP address manually
	range -= 2; // subtract the network address and the broadcast address

	// try all possible ip addresses in ascending order
	uint32_t dest = (ifip & ifmask) + 1;
	uint32_t last = dest + range - 1;
	if (arg_debug)
		printf("Trying IP address range from %d.%d.%d.%d to %d.%d.%d.%d\n",
			PRINT_IP(dest), PRINT_IP(last));
	while (dest <= last) {
		if (dest == ifip) {
			dest++;
			continue;
		}
		uint32_t rv = arp_check(dev, dest, ifip);
		if (!rv)
			return dest;
		dest++;
	}

	return 0;
}

// assign an IP address using the specified IP address or the ARP mechanism
uint32_t arp_assign(const char *dev, uint32_t ifip, uint32_t ifmask) {
	uint32_t ip = 0;

	// try two random IP addresses
	ip = arp_random(dev, ifip, ifmask);
	if (!ip)
		ip = arp_random(dev, ifip, ifmask);
	// try all possible IP addresses one by one
	if (!ip)
		ip = arp_sequential(dev, ifip, ifmask);
	if (!ip) {
		fprintf(stderr, "Error: cannot assign an IP address; it looks like all of them are in use\n");
		logerr("Cannot assign an IP address; it looks like all of them are in use");
		exit(1);
	}
	printf("%d.%d.%d.%d IP address assigned to the sandbox\n", PRINT_IP(ip));
	
	return ip;
}
