//
//  hw2_server.c
//  
//
//  Created by borute on 10/15/19.
//

//#include "hw2_server.h"
#include "unpv13e/lib/unp.h"
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>



int main(int argc, char* argv[]){
    //setup variables
    srand(atoi(argv[1]));
    
    //setup the secret word

    
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    bzero(&servaddr, sizeof(servaddr));
    int player_count = 0;
    
    //create socket
    
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("socket create failed");
        return 0;
    }
    
    //set up server
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[2]));
    
    //bing socket to server
    
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(listenfd, 10);

    socklen_t len = sizeof(cliaddr);

    connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

    char buffer[256];
    int n;
    fd_set fds;
    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(connfd, &fds);
        FD_SET(0, &fds);
        select(connfd + 1, &fds, NULL, NULL, NULL);
        if (FD_ISSET(connfd, &fds))
        {
            bzero(buffer, 256);
            printf("%ld\n", read(connfd, buffer, 256));
            printf("buffer: %s\n", buffer);
            bzero(buffer, 256);
        }
        if (FD_ISSET(0, &fds))
        {
            bzero(buffer, 256);
            fgets(buffer, 256, stdin);
            if (buffer[strlen(buffer) - 1] == '\n')
            {
                buffer[strlen(buffer) - 1] = '\0';
            }
            send(connfd, buffer, strlen(buffer), 0);
            bzero(buffer, 256);
        }
    }
        
    /*while(1){
        
        //set up the fd_set for select()
        
        FD_SET(listenfd, &fds);
        int max_playerfd = -1;
        for(int a = 0; a < 5; a++){
                if(player_fds[a] > -1){
                    FD_SET(player_fds[a], &fds);
                    if(player_fds[a] > max_playerfd){
                        max_playerfd = player_fds[a];
                    }
                }
        }
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 5;
        int maxfd = max(max_playerfd, listenfd) + 1;
        
        //select
        
        select(maxfd, &fds, NULL, NULL, NULL);
        
        if(FD_ISSET(listenfd, &fds)){
            
            //if there is incoming new connection from client
            
            printf("New client detected.\n");
            socklen_t len = sizeof(cliaddr);
            int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            
            //make sure this client can join the game
            
            if(player_count >= 5){
                close(connfd);
                continue;
            }
            
            //player_count increment
            
            player_count++;
            FD_SET(connfd, &fds);
            char welcome_message[1024];
            for(int a = 0; a < 1024; a++){
                welcome_message[a] = '\0';
            }
            
            //greeting
            
            sprintf(welcome_message, "Welcome to Guess the Word, please enter your username.");
            printf("Welcome message sent.\n");
            send(connfd, welcome_message, strlen(welcome_message), 0);
            for(int a = 0; a < 1025; a++){
                buffer[a] = '\0';
            }
            
            //read username
            
            read(connfd, buffer, sizeof(buffer));
            FD_CLR(connfd, &fds);
            buffer[strlen(buffer) - 1] = '\0';
            printf("strlen of username is: %lu\n", strlen(buffer));
            bool name_used = false;
            
            //check if username is used
            
            for(int a = 0; a < 5; a++){
                if(!slot_available[a]){
                    if(strcmp(buffer, usernames[a]) == 0){
                        name_used = true;
                        char message[1025];
                        for(int c = 0; c < 1025; c++){
                            message[c] = '\0';
                        }
                        sprintf(message, "Username %s is already taken, please enter a different username.", usernames[a]);
                        send(connfd, message, strlen(message), 0);
                        printf("Client requested a used username: %s\n", buffer);
                        break;
                    }
                }
            }
            
            //fill in user data and prompt the client to start playing
            
            if(!name_used){
                for(int a = 0; a < 5; a++){
                    
                    //find first empty slot for the player
                    
                    if(slot_available[a]){
                        
                        //fill in the details
                        //and send prompts
                        
                        printf("New user (%s) has joined.\n", buffer);
                        slot_available[a] = false;
                        player_fds[a] = connfd;
                        free(usernames[a]);
                        usernames[a] = calloc(strlen(buffer) + 1, sizeof(char));
                        sprintf(usernames[a], "%s", buffer);
                        usernames[a][strlen(buffer)] = '\0';
                        has_username[a] = true;
                        char message[1025];
                        for(int c = 0; c < 1025; c++){
                            message[c] = '\0';
                        }
                        sprintf(message, "Let's start playing, %s", usernames[a]);
                        send(connfd, message, strlen(message), 0);
                        //fighting against a random bug on submitty
                        usleep(100);
                        char message2[1025];
                        for(int c = 0; c < 1025; c++){
                            message2[c] = '\0';
                        }
                        sprintf(message2, "There are %d player(s) playing. The secret word is %lu letter(s).", player_count, strlen(secret));
                        send(connfd, message2, strlen(message2), 0);
                        break;
                    }
                }
            }
            
            //username is used, but the slot is still being occupied
            //while the player tries to use another username
            
            else if(name_used){
                for(int a = 0; a < 5; a++){
                    if(slot_available[a]){
                        slot_available[a] = false;
                        player_fds[a] = connfd;
                        break;
                    }
                }
            }
        }
        
        //checking incoming data from clients
        
        for(int a = 0; a < 5; a++){
            
            //if a slot is being used, check for incoming data from the
            //corresponding file descriptor of that slot
            
            if(!slot_available[a]){
                
                //if something is coming in
                
                if(FD_ISSET(player_fds[a], &fds)){
                    for(int c = 0; c < 1025; c++){
                        buffer[c] = '\0';
                    }
                    //read the data
                    int bytes_read = read(player_fds[a], buffer, sizeof(buffer));
                    FD_CLR(player_fds[a], &fds);
                    buffer[strlen(buffer) - 1] = '\0';
                    printf("%d\n", bytes_read);
                    printf("length of guess word: %lu\n", strlen(buffer));
                    
                    //if read returns anything less than or equals to 0,
                    //the client has disconnected
                    
                    if (bytes_read <= 0){
                        //close connection and erase user data
                        printf("Player %s disconnected.\n", usernames[a]);
                        close(player_fds[a]);
                        player_fds[a] = -1;
                        slot_available[a] = true;
                        has_username[a] = false;
                        player_count--;
                        continue;
                    }
                    
                    //if the client has an username (is playing the game)
                    //and actually sent data, proceed to guess
                    
                    if(has_username[a] && bytes_read > 0){
                        
                        //handle guessing here
                        
                        bool game_over = false;
                        printf("User %s (%d) has guessed: %s\n", usernames[a], a, buffer);
                        
                        //the matching results
                        
                        struct match_number result = word_match(secret, buffer);
                        
                        
                        char message[2048];
                        for(int c = 0 ; c < 2048; c++){
                            message[c] = '\0';
                        }
                        
                        //if the guess word has invalid length
                        
                        if(result.correct == -1){
                            sprintf(message, "Invalid guess length. The secret word is %lu letter(s).", strlen(secret));
                            send(player_fds[a], message, strlen(message), 0);
                            continue;
                        }
                        
                        //if guessed correctly, the game_over flag is marked as true
                        //and get a new secret word
                        else if(result.place_correct == strlen(secret)){
                            sprintf(message, "%s has correctly guessed the word %s", usernames[a], secret);
                            free(secret);
                            secret = get_secret(dic, dic_size);
                            printf("Game over, new word (%s) has been selected.\n", secret);
                            game_over = true;
                        }
                        
                        //nice try
                        
                        else{
                            sprintf(message, "%s guessed %s: %d letter(s) were correct and %d letter(s) were correctly placed.", usernames[a], buffer, result.correct, result.place_correct);
                        }
                        
                        //broadcast the guess made by the current player to all players
                        
                        for(int b = 0; b < 5; b++){
                            if(has_username[b]){
                                send(player_fds[b], message, strlen(message), 0);
                                //if the game_over flag is true,
                                //disconnect all players and erase all data
                                if(game_over){
                                    close(player_fds[b]);
                                    player_fds[b] = -1;
                                    slot_available[b] = true;
                                    has_username[b] = false;
                                    player_count--;
                                }
                            }
                        }
                    }
                    
                    //if the player doesn't have a username,
                    //then it's still looking for a valid one
                    //to join the game.
                    
                    else{
                        bool name_used = false;
                        
                        //check if name has been used
                        
                        for(int b = 0; b < 5; a++){
                            if(!slot_available[b]){
                                if(strcmp(buffer, usernames[b]) == 0){
                                    name_used = true;
                                    char message[1025];
                                    for(int c = 0; c < 1025; c++){
                                        message[c] = '\0';
                                    }
                                    strcpy(message, "Username ");
                                    strcat(message, buffer);
                                    strcat(message, " is already taken, please enter a different username.");
                                    send(player_fds[a], message, strlen(message), 0);
                                    printf("Client requested a used username: %s\n", buffer);
                                    break;
                                }
                            }
                        }
                        
                        //if the name is not used, join the game
                        
                        if(!name_used){
                            printf("New user (%s) has joined.\n", buffer);
                            free(usernames[a]);
                            usernames[a] = calloc(strlen(buffer) + 1, sizeof(char));
                            sprintf(usernames[a], "%s", buffer);
                            usernames[a][strlen(buffer)] = '\0';
                            has_username[a] = true;
                            char message[1025];
                            for(int c = 0; c < 1025; c++){
                                message[c] = '\0';
                            }
                            sprintf(message, "Let's start playing, %s", usernames[a]);
                            send(player_fds[a], message, strlen(message), 0);
                            //fighting against a random bug on submitty
                            usleep(100);
                            char message2[1025];
                            for(int c = 0; c < 1025; c++){
                                message2[c] = '\0';
                            }
                            sprintf(message2, "There are %d player(s) playing. The secret word is %lu letter(s).", player_count, strlen(secret));
                            send(player_fds[a], message2, strlen(message2), 0);
                        }
                    }
                }
            }
        }
    }
    */
}
