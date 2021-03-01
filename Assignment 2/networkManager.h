// Module to setup network configuration for UDP

#ifndef _NETWORK_MANAGER_H
#define _NETWORK_MANAGER_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <sys/types.h>

#define PORT 12345
#define HOST "beaglebone"
#define MAXCOMMANDBUFLEN 10

// default values for UDP sockets
int sockfd;
struct sockaddr_in servaddr, cliaddr;

int networkConfigged = 0;
int Network_init();

#endif