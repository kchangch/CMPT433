#include "networkManager.h"

int Network_init() {
	// Creating socket file descriptor 
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		perror("Could not create a new socket for listening.\n");
		return 1;
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
	
	servaddr.sin_family = AF_INET; // IPv4 
	servaddr.sin_addr.s_addr = INADDR_ANY; 
	servaddr.sin_port = htons(PORT); 
	
	if (
        bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0
    ) {
		perror("Could not bind to specified port\n");
		return 1;
	}

    return 0;
}
