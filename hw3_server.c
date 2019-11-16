//
//  hw2_server.c
//  
//
//  Created by borute on 10/15/19.
//
//#include "unp.h"
#include "unpv13e/lib/unp.h"
#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <float.h>


struct BaseStation{
    char* name;
    int x_pos;
    int y_pos;
    int num_connections;
    char** connections;
};

struct Sensor{
    char* name;
    int range;
    int x_pos;
    int y_pos;
    int fd;
};

void addBStoBSlist(struct BaseStation** BSs, char* line, int bs_index);
int addClientFDsToFDset(fd_set* fds, int count, int* clientFDs);
char** parseMessage(char* line);
bool reachable(float ax, float ay, float bx, float by, float range);

int main(int argc, char* argv[]){
    //setup variables
    //srand(atoi(argv[1]));
    
    //setup the secret word

    
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    bzero(&servaddr, sizeof(servaddr));
    
    //create socket
    
    int bs_count = 0;
    int sensor_count = 0;
    struct BaseStation** baseStations = calloc(32, sizeof(struct BaseStation*));
    struct Sensor** sensors = calloc(32, sizeof(struct Sensor*));
    for(int a = 0; a < 32; a++){
        sensors[a] = calloc(1, sizeof(struct Sensor));
        sensors[a] -> name = calloc(64, sizeof(char));
        (sensors[a] -> name)[0] = 'u';
        (sensors[a] -> name)[1] = 'n';
        (sensors[a] -> name)[2] = 'd';
        (sensors[a] -> name)[3] = 'e';
        (sensors[a] -> name)[4] = 'f';
        (sensors[a] -> name)[5] = '\0';
        sensors[a] -> range = -1;
        sensors[a] -> x_pos = -1;
        sensors[a] -> y_pos = -1;
        sensors[a] -> fd = -1;
    }
    
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("socket create failed");
        return 0;
    }
    
    //set up server
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));
    
    FILE* base_station_file = fopen(argv[2], "r");
    
    char* line = calloc(1025, sizeof(char));
    size_t size = 1025;

    
    /*
     * read the base stations
     */
    while(getline(&line, &size, base_station_file) != -1){
        addBStoBSlist(baseStations, line, bs_count);
        bs_count++;
    }
    
    /*
    printf("Checking stations...\n\n");
    for(int a = 0; a < bs_count; a++){
        struct BaseStation* bs = baseStations[a];
        printf("base station ID: %s, X: %d, Y: %d, number of connections: %d, connected stations: ", bs -> name, bs -> x_pos, bs -> y_pos, bs -> num_connections);
        for(int b = 0; b < bs -> num_connections; b++){
            printf("%s ", bs -> connections[b]);
        }
        printf("\n");
    }
    printf("Station check done!\n\n");
    */
    
    //bind socket to server
    
    bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
    listen(listenfd, 10);

    socklen_t len = sizeof(cliaddr);

    //connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);

    int clientFDs[32];
    char buffer[5120];
    int n;
    fd_set fds;
    while (1)
    {
        //set up the fd_set
        FD_ZERO(&fds);
        FD_SET(listenfd, &fds);
        //stdin
        FD_SET(0, &fds);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 5;
        
        int max_client_fd = addClientFDsToFDset(&fds, sensor_count, clientFDs);
        //printf("gg\n");
        
        select((max(max_client_fd, listenfd) + 1), &fds, NULL, NULL, NULL);
        //
        //new connection
        
        if (FD_ISSET(listenfd, &fds))
        {
            //printf("New client!\n");
            socklen_t len = sizeof(cliaddr);
            connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &len);
            clientFDs[sensor_count] = connfd;
            sensor_count++;
            
            continue;
        }
        
        
        //stdin
        if (FD_ISSET(0, &fds)){
            char aaa[25];
            scanf("%s", aaa);
            break;
        }
        
        //receives message/requests from client
        for(int client = 0; client < sensor_count; client++){
            if(FD_ISSET(clientFDs[client], &fds)){
                int bytes_read = read(clientFDs[client], buffer, sizeof(buffer));
                buffer[bytes_read] = '\0';
                //printf("\nMessage received: %s\n", buffer);
                
                char** arguments = parseMessage(buffer);
                
                if(strcmp(arguments[0], "UPDATEPOSITION") == 0){
                    bool updated = false;
                    for(int a = 0; a < sensor_count; a++){
                        if(strcmp(sensors[a] -> name, arguments[1]) == 0){
                            updated = true;
                            sensors[a] -> range = atoi(arguments[2]);
                            sensors[a] -> x_pos = atoi(arguments[3]);
                            sensors[a] -> y_pos = atoi(arguments[4]);
                            break;
                        }
                    }
                    if(!updated){
                        for(int a = 0; a < sensor_count; a++){
                            if(strcmp(sensors[a] -> name, "undef") == 0){
                                free(sensors[a] -> name);
                                sensors[a] -> name = calloc(strlen(arguments[1]) + 1, sizeof(char));
                                memcpy(sensors[a] -> name, arguments[1], strlen(arguments[1]));
                                (sensors[a] -> name)[strlen(arguments[1])] = '\0';
                                sensors[a] -> range = atoi(arguments[2]);
                                sensors[a] -> x_pos = atoi(arguments[3]);
                                sensors[a] -> y_pos = atoi(arguments[4]);
                                sensors[a] -> fd = clientFDs[client];
                                break;
                            }
                        }
                    }
                    float current_x = (float)atoi(arguments[3]);
                    float current_y = (float)atoi(arguments[4]);
                    int reachable_count = 0;
                    char reachables[2048];
                    char response[4096];
                    reachables[0] = '\0';
                    for(int a = 0; a < bs_count; a++){
                        char temp[256];
                        float target_x = (float)(baseStations[a] -> x_pos);
                        float target_y = (float)(baseStations[a] -> y_pos);
                        if(reachable(current_x, current_y, target_x, target_y, (float)atoi(arguments[2]))){
                            reachable_count++;
                            sprintf(temp, "%s %d %d ", baseStations[a] -> name, baseStations[a] -> x_pos, baseStations[a] -> y_pos);
                            strcat(reachables, temp);
                        }
                    }
                    for(int a = 0; a < sensor_count; a++){
                        char temp[256];
                        float target_x = (float)(sensors[a] -> x_pos);
                        float target_y = (float)(sensors[a] -> y_pos);
                        if(reachable(current_x, current_y, target_x, target_y, (float)atoi(arguments[2]))){
                            reachable_count++;
                            sprintf(temp, "%s %d %d ", sensors[a] -> name, sensors[a] -> x_pos, sensors[a] -> y_pos);
                            strcat(reachables, temp);
                        }
                    }
                    
                    sprintf(response, "REACHABLE %d %s", reachable_count, reachables);
                    response[strlen(response) - 1] = '\0';
                    
                    //printf("response to %s: %s\n\n", arguments[1], response);
                    
                    
                    if(send(clientFDs[client], response, strlen(response), 0) == -1){
                        printf("send error (REACHABLE)\n");
                    }
                    
                    /*
                    printf("Updated list of sensors: \n");
                    
                    for(int a = 0; a < sensor_count; a++){
                        printf("ID: %s, range: %d, Xpos: %d, Ypos: %d\n", sensors[a] -> name, sensors[a] -> range, sensors[a] -> x_pos, sensors[a] -> y_pos);
                    }
                    printf("\n");
                    */
                }
                
                if(strcmp(arguments[0], "WHERE") == 0){
                    bool responded = false;
                    char response[512];
                    for(int a = 0; a < bs_count; a++){
                        if(strcmp(baseStations[a] -> name, arguments[1]) == 0){
                            sprintf(response, "THERE %s %d %d", baseStations[a] -> name, baseStations[a] -> x_pos, baseStations[a] -> y_pos);
                            responded = true;
                            break;
                        }
                    }
                    if(!responded){
                        for(int a = 0; a < sensor_count; a++){
                            if(strcmp(sensors[a] -> name, arguments[1]) == 0){
                                sprintf(response, "THERE %s %d %d", sensors[a] -> name, sensors[a] -> x_pos, sensors[a] -> y_pos);
                                responded = true;
                                break;
                            }
                        }
                    }
                    
                    if(send(clientFDs[client], response, strlen(response), 0) == -1){
                        printf("send error(THERE)\n");
                    }
                    
                    
                    //printf("response: %s\n\n", response);
                }
                if(strcmp(arguments[0], "DATAMESSAGE") == 0){
                    //will break out of the loop when a message is sent to a sensor or reaches a deadend
                    while(1){
                        /*
                        printf("\ndatamsg: ");
                        for(int a = 0; a <= atoi(arguments[4]) + 4; a++){
                            printf("%s ", arguments[a]);
                        }
                        printf("\n\n");
                        //printf("gg1\n");
                        */
                        
                        bool is_base = false;
                        bool is_sensor = false;
                        int targetFD = -1;
                        struct BaseStation* bs;
                        for(int a = 0; a < bs_count; a++){
                            //printf("gg2\n");

                            if(strcmp(baseStations[a] -> name, arguments[2]) == 0){

                                bs = baseStations[a];
                                is_base = true;
                                break;
                            }
                        }
                        if(!is_base){
                            for(int a = 0; a < sensor_count; a++){
                                if(strcmp(sensors[a] -> name, arguments[2]) == 0){
                                    is_sensor = true;
                                    targetFD = sensors[a] -> fd;
                                    break;
                                }
                            }
                        }
                        if(is_sensor){
                            if(send(targetFD, buffer, strlen(buffer), 0) == -1){
                                printf("send error(DATAMESSAGE)\n");
                            }
                            break;
                        }
                        else if(is_base){
                            float bs_x = (float)bs -> x_pos;
                            float bs_y = (float)bs -> y_pos;
                            if(strcmp(arguments[3], bs -> name) == 0){
                                printf("%s: Message from %s to %s successfully received.\n", bs -> name, arguments[1], arguments[3]);
                                break;
                            }
                            else{
                                bool dest_found = false;
                                //find the position of the destination
                                float dest_x;
                                float dest_y;
                                for(int a = 0; a < bs_count; a++){
                                    //
                                    if(strcmp(arguments[3], baseStations[a] -> name) == 0){
                                        dest_x = baseStations[a] -> x_pos;
                                        dest_y = baseStations[a] -> y_pos;
                                        dest_found = true;
                                        break;
                                    }
                                }
                                if(!dest_found){
                                    for(int a = 0; a < sensor_count; a++){
                                        if(strcmp(arguments[3], sensors[a] -> name) == 0){
                                            dest_x = sensors[a] -> x_pos;
                                            dest_y = sensors[a] -> y_pos;
                                            break;
                                        }
                                    }
                                }
                                
                                //find the loactions that this station can reach
                                int next_index = 0;
                                char** next = calloc(32, sizeof(char*));
                                float dists[32];
                                for(int a = 0; a < bs -> num_connections; a++){
                                    next[next_index] = (bs -> connections)[a];
                                    for(int b = 0; b < bs_count; b++){
                                        if(strcmp((bs -> connections)[a], baseStations[b] -> name) == 0){
                                            float begin_x = baseStations[b] -> x_pos;
                                            float begin_y = baseStations[b] -> y_pos;
                                            dists[next_index] = sqrtf((begin_x - dest_x) * (begin_x - dest_x) + (begin_y - dest_y) * (begin_y - dest_y));
                                            break;
                                        }
                                    }
                                    next_index++;
                                }
                                for(int a = 0; a < sensor_count; a++){
                                    if(reachable(sensors[a] -> x_pos, sensors[a] -> y_pos, bs_x, bs_y, sensors[a] -> range)){
                                        float begin_x = sensors[a] -> x_pos;
                                        float begin_y = sensors[a] -> y_pos;
                                        next[next_index] = sensors[a] -> name;
                                        dists[next_index] = sqrtf((begin_x - dest_x) * (begin_x - dest_x) + (begin_y - dest_y) * (begin_y - dest_y));
                                        next_index++;
                                    }
                                }
                                
                                //now we have all the sensors and stations that this station can reach
                                float shortest_dist = FLT_MAX;
                                char* next_name;
                                bool dead = true;
                                //find the next nearest available node
                                for(int a = 0; a < next_index; a++){
                                    bool skip = false;
                                    //skip the ones in HopList
                                    for(int b = 5; b < (5 + atoi(arguments[4])); b++){
                                        if(strcmp(next[a], arguments[b]) == 0){
                                            skip = true;
                                            break;
                                        }
                                    }
                                    if(skip){
                                        continue;
                                    }
                                    
                                    //tie
                                    if(fabs(dists[a] - shortest_dist) < FLT_EPSILON){
                                        if(strcmp(next[a], next_name) < 0){
                                            next_name = next[a];
                                            shortest_dist = dists[a];
                                        }
                                    }
                                    else if(dists[a] < shortest_dist){
                                        //printf("!\n");
                                        dead = false;
                                        shortest_dist = dists[a];
                                        next_name = next[a];
                                    }
                                }
                                printf("%s: Message from %s to %s being forwarded through %s\n", bs -> name, arguments[1], arguments[3], arguments[2]);
                                if(dead){
                                    printf("%s: Message from %s to %s could not be delivered.\n", bs -> name, arguments[1], arguments[3]);
                                    break;
                                }
                                //printf("The message will be forwarded to: %s\n\n", next_name);
                                int count_space = 0;
                                for (int pos = 0; pos < strlen(buffer); pos++)
                                {
                                    if (buffer[pos] == ' ') {count_space++;}
                                    if (count_space == 4)
                                    {
                                        pos++;
                                        buffer[pos] = buffer[pos] + 1;
                                        break;
                                    }
                                }
                                strcat(buffer, " ");
                                strcat(buffer, bs -> name);
                                printf("%s\n", buffer);
                                arguments[2] = next_name;
                                arguments[5 + atoi(arguments[4])] = calloc(strlen(bs -> name) + 1, sizeof(char));
                                sprintf(arguments[5 + atoi(arguments[4])], "%s", bs -> name);
                                int hopCount = atoi(arguments[4]);
                                arguments[4] = calloc(10, sizeof(char));
                                sprintf(arguments[4], "%d", hopCount + 1);
                            }
                        }
                    }
                    
                }
            }
        }
    }
    for(int client = 0; client < sensor_count; client++)
    {
        close(clientFDs[client]);
    }
    close(listenfd);
    return 1;
}

int addClientFDsToFDset(fd_set* fds, int count, int* clientFDs){
  
    int maxfd = -1;
    int* ptr = clientFDs;
    for(int a = 0; a < count; a++){
        FD_SET(*ptr, fds);
        if(*ptr > maxfd){
            maxfd = *ptr;
        }
        ptr++;
    }
    return maxfd;
}

void addBStoBSlist(struct BaseStation** BSs, char* line, int bs_index){
    int arg_count = 0;
    int argument_index = 0;
    int current_index = 0;
    int connection_index = 0;
    char* line_ptr = line;
    struct BaseStation* bs = calloc(1, sizeof(struct BaseStation));
    while(*line_ptr != '\0' && *line_ptr != '\n'){
        //stops at space

        if(*line_ptr == ' '){
            //name of the base station
            if(arg_count == 0){
                bs -> name = calloc((current_index - argument_index) + 1, sizeof(char));
                memcpy(bs -> name, line, current_index - argument_index);
                (bs -> name)[current_index - argument_index] = '\0';
                //current_index++;
                argument_index = current_index + 1;
                arg_count++;
                //line_ptr++;
            }
            
            //x pos
            else if(arg_count == 1){
                bs -> x_pos = atoi(line + argument_index);
                //current_index++;
                argument_index = current_index + 1;
                arg_count++;
                //line_ptr++;
            }
            
            //y pos
            else if(arg_count == 2){
                bs -> y_pos = atoi(line + argument_index);
                //current_index++;
                argument_index = current_index + 1;
                arg_count++;
                //line_ptr++;
            }
            
            //# of connections
            else if(arg_count == 3){
                bs -> num_connections = atoi(line + argument_index);
                bs -> connections = calloc(bs -> num_connections, sizeof(char*));
                //current_index++;
                argument_index = current_index + 1;
                arg_count++;
                //line_ptr++;
            }
            
            else{
                bs -> connections[connection_index] = calloc((current_index - argument_index) + 1, sizeof(char));
                memcpy(bs -> connections[connection_index], line + argument_index, current_index - argument_index);
                (bs -> connections[connection_index])[current_index - argument_index] = '\0';
                connection_index++;
                //current_index++;
                argument_index = current_index + 1;
                arg_count++;
                //line_ptr++;
            }
        }
        current_index++;
        line_ptr++;
    }
    //add the last argument in the line to the list of connected stations
    bs -> connections[connection_index] = calloc((current_index - argument_index) + 1, sizeof(char));
    memcpy(bs -> connections[connection_index], line + argument_index, current_index - argument_index);
    (bs -> connections[connection_index])[current_index - argument_index] = '\0';
    BSs[bs_index] = bs;
}

//converts a line into a list of chats

char** parseMessage(char* msg){
    char** rtn_val = calloc(64, sizeof(char*));
    int rtn_val_index = 0;
    char* ptr = msg;
    int current_index = 0;
    int argument_index = 0;
    while(*ptr != '\0' && *ptr != '\n'){
        if(*ptr == ' '){
            rtn_val[rtn_val_index] = calloc((current_index - argument_index) + 1, sizeof(char));
            memcpy(rtn_val[rtn_val_index], msg + argument_index, current_index - argument_index);
            rtn_val[rtn_val_index][current_index - argument_index] = '\0';
            argument_index = current_index + 1;
            rtn_val_index++;
        }
        current_index++;
        ptr++;
    }
    
    rtn_val[rtn_val_index] = calloc((current_index - argument_index) + 1, sizeof(char));
    memcpy(rtn_val[rtn_val_index], msg + argument_index, current_index - argument_index);
    rtn_val[rtn_val_index][current_index - argument_index] = '\0';
    return rtn_val;
}

bool reachable(float ax, float ay, float bx, float by, float range){
    if((fabs(ax - bx) < FLT_EPSILON) && (fabs(ay - by) < FLT_EPSILON)){
        return false;
    }
    float dist = sqrtf((ax - bx) * (ax - bx) + (ay - by) * (ay - by));
    return dist < range || (fabs(dist - range) < FLT_EPSILON);
}

