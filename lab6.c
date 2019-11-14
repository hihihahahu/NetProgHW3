#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "lib/unp.h"

int main(int argc, char* argv[]){
	struct addrinfo* results;
	char ip_addr[256];
	char ip_addr6[256];
	
	if(argc < 2){
		printf("Arguments invalid\n");
		return 0;
	}
	
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	
	/*
		IPv4 addresses
	*/
	
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	
	//char* error = calloc(128, sizeof(char));
	
	int error = getaddrinfo(argv[1], NULL, &hints, &results);
	
	if(error){
		fprintf(stderr, "getaddrinfo() failed: %s\n", gai_strerror(error));
		return 0;
	}
	
	struct addrinfo* result;
	//struct addrinfo* temp = results;
	
	/*
	int num_ipv4 = 0;
	
	while(temp != NULL){
		temp = temp -> ai_next;
		num_ipv4++;
	}
	
	printf("number of ipv4 connections: %d\n", num_ipv4);	
	*/
	
	for(result = results; result != NULL; result = result -> ai_next){
		//printf("?\n");
		//struct in_addr* addr;
		getnameinfo(result -> ai_addr, result -> ai_addrlen, ip_addr, sizeof(ip_addr), NULL, 0, NI_NUMERICHOST);
		printf("%s\n", ip_addr);
		//free(addr);
	}
	
	freeaddrinfo(results); 
	
	/*
		IPv6 addresses
	*/
	
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_STREAM;
	
	if(getaddrinfo(argv[1], NULL, &hints, &results)){
		fprintf(stderr, "getaddrinfo() failed\n");
		return 0;
	}
	
	for(struct addrinfo* result = results; result != NULL; result = result -> ai_next){
		//printf("?\n");
		//struct in_addr* addr;
		struct sockaddr_in6* ipv6_adr = (struct sockaddr_in6*)result -> ai_addr;
		
		//char* ip6 = calloc(256, sizeof(char));
		
		inet_ntop(AF_INET6, &((ipv6_adr)), ip_addr6, sizeof(ip_addr6));
		
		printf("%s\n", ip_addr6);
	}
	
	freeaddrinfo(results); 
	
	return 0;
	
}