//
//  hw2_client.c
//  
//
//  Created by borute on 10/15/19.
//

#include "hw2_client.h"
#include "lib/unp.h"
#include <stdio.h>
int main(int argc, char* argv[]){
    int sockfd;
    struct sockaddr_in servaddr;
    //int addrlen;
    int port = atoi(argv[2]);
    
    fd_set fds;
    
    int username_len = 0;
    char* username;
    username = calloc(1025, sizeof(char));
    username[0] = '\0';
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("socket create failed\n");
        return 0;
    }
    
    bzero(&servaddr, sizeof(servaddr)); 
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    
    if(connect(sockfd, (struct sockaddr*)& servaddr, sizeof(servaddr)) < 0){
        printf("connect failed\n");
        return 0;
    }
    
    char server_response[1025];
    
    
    while(1){
        FD_SET(sockfd, &fds);
        select(sockfd + 1, &fds, NULL, NULL, NULL);
        if(FD_ISSET(sockfd, &fds)){
            for(int a = 0; a < 1025; a++){
                server_response[a] = '\0';
            }
            read(sockfd, server_response, sizeof(server_response));
            printf("%s\n", server_response);
            printf("G G\n");
            char* temp;
            temp = calloc(2049, sizeof(char));
            strcpy(temp, "Username ");
            strcat(temp, username);
            strcat(temp, " is already taken, please enter a different username");
            temp[strlen(temp)] = '\0';
            if(strcmp(server_response, "Welcome to Guess the Word, please enter your username.") == 0 || strcmp(server_response, temp) == 0){
                fgets(server_response, sizeof(server_response), stdin);
                //int index = 0;
                
                server_response[strlen(server_response)] = '\0';
                username = server_response;
                username_len = strlen(username);
                write(sockfd, server_response, strlen(server_response) + 1);
            }
            else if(strcmp((server_response), "GGWP") == 0){
                free(temp);
                break;
            }
            else if(strcmp((server_response), "Too many players atm.") == 0){
                free(temp);
                break;
            }
            else if(strlen(server_response) > 0){
                //fflush(stdin);
                fgets(server_response, sizeof(server_response), stdin);
                //int index = 0;
                
                server_response[strlen(server_response)] = '\0';
                write(sockfd, server_response, strlen(server_response) + 1);
            }
        }
        
    }
    free(username);
    close(sockfd);
    return 0;
}
