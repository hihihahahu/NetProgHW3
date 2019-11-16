//
//  hw2_client.c
//  
//
//  Created by borute on 10/15/19.
//
//#include "unp.h"
#include "unpv13e/lib/unp.h"
#include <stdio.h>
#include <math.h>

float x_pos = 0;
float y_pos = 0;
int range = 0;
char* sensor_id;

struct station
{
    char* id;
    float x;
    float y;
    float dist;
};

void move(int sockfd, char* buffer)
{
    strtok(buffer, " ");
    x_pos = atof(strtok(NULL, " "));
    y_pos = atof(strtok(NULL, " "));
    char update[200];
    bzero(update, 200);
    sprintf(update, "UPDATEPOSITION %s %d %f %f", sensor_id, range, x_pos, y_pos);
    send(sockfd, update, strlen(update), 0);
    bzero(update, 200);
    while (update[0] != 'R')
    {
        read(sockfd, update, 200);
    }
}

void get_reach_list(char* buffer, int* length, struct station* reach_list)
{
    strtok(buffer, " ");
    *length = atoi(strtok(NULL, " "));
    for (int i = 0; i < *length; i++)
    {
        reach_list[i].id = strtok(NULL, " ");
        reach_list[i].x = atof(strtok(NULL, " "));
        reach_list[i].y = atof(strtok(NULL, " "));
    }
}

float distance(float x1, float y1, float x2, float y2)
{
    return sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

int cmpfunc(const void* a, const void* b)
{
    struct station* s1 = (struct station *) a;
    struct station* s2 = (struct station *) b;
    float precision = 0.00001;
    if (fabs(s1 -> dist - s2 -> dist) < precision)
    {
        return strcmp(s1 -> id, s2 -> id);
    }
    if (s1 -> dist - s2 -> dist < 0) {return -1;}
    return 1;
}

struct station* get_send_list(char** hop_list, int hop_num, struct station* reach_list, int reach_num, int* size, float dest_x, float dest_y)
{
    *size = 0;
    struct station* ans = (struct station*)calloc(reach_num, sizeof(struct station));
    for (int i = 0; i < reach_num; i++)
    {
        int ishop = 0;
        for (int j = 0; j < hop_num; j++)
        {
            if (strcmp(hop_list[j], reach_list[i].id) == 0)
            {
                ishop = 1;
                break;
            }
        }

        if (!ishop)
        {
            ans[*size].id = reach_list[i].id;
            ans[*size].dist = distance(reach_list[i].x, reach_list[i].y, dest_x, dest_y);
            *size = *size + 1;
        }
    }
    return ans;
}

void senddata(int sockfd, char* buffer) 
{
    char update[200] = {0};
    char datamsg[200] = {0};
    struct station* reach_list = (struct station*)calloc(30, sizeof(struct station));
    int reach_num = 0;

    strtok(buffer, " ");
    char* dest_id = strtok(NULL, " ");
    bzero(update, 200);

    //Send UPDATEPOSITION
    sprintf(update, "UPDATEPOSITION %s %d %f %f", sensor_id, range, x_pos, y_pos);
    send(sockfd, update, strlen(update), 0);

    //Get list of station reachable
    bzero(update, 200);
    while (update[0] != 'R')
    {
        bzero(update, 200);
        read(sockfd, update, 200);
    }
    get_reach_list(update, &reach_num, reach_list);

    //If no reachable station
    if (reach_num == 0)
    {
        printf("%s: Message from %s to %s could not be delivered.\n", sensor_id, sensor_id, dest_id);
        return;
    }

    printf("%s: Sent a new message bound for %s.\n", sensor_id, dest_id);
    //If destination is reachable
    for (int i = 0; i < reach_num; i++)
    {
        if (strcmp(reach_list[i].id, dest_id) == 0)
        {
            sprintf(datamsg, "DATAMESSAGE %s %s %s 1 %s", sensor_id, dest_id, dest_id, sensor_id);
            send(sockfd, datamsg, strlen(datamsg), 0);
            return;
        }
    }

    //Send where to get pos of destination
    char wheremsg[200];
    bzero(wheremsg, 200);
    sprintf(wheremsg, "WHERE %s", dest_id);
    send(sockfd, wheremsg, strlen(wheremsg), 0);
    bzero(wheremsg, 200);
    while (wheremsg[0] != 'T')
    {
        bzero(wheremsg, 200);
        read(sockfd, wheremsg, 200);
    }
    strtok(wheremsg, " ");
    strtok(NULL, " ");
    float dest_x = atof(strtok(NULL, " "));
    float dest_y = atof(strtok(NULL, " "));

    //Get list of sendable node
    int s_num;
    struct station* s_list = get_send_list(NULL, 0, reach_list, reach_num, &s_num, dest_x, dest_y);

    qsort(s_list, s_num, sizeof(struct station), cmpfunc);

    /*
    for (int i = 0; i < s_num; i++)
    {
        printf("%s %f\n", s_list[i].id, s_list[i].dist);
    }
    */
    bzero(datamsg, 200);
    sprintf(datamsg, "DATAMESSAGE %s %s %s 1 %s", sensor_id, s_list[0].id, dest_id, sensor_id);
    send(sockfd, datamsg, strlen(datamsg), 0);
    return;
}

void do_data(int sockfd, char* buffer)
{
    char datamsg[200];
    char update[200];
    struct station* reach_list = (struct station*)calloc(30, sizeof(struct station));
    int reach_num = 0;

    //Get origin id, destination id
    strtok(buffer, " ");    
    char* origin_id = strtok(NULL, " ");
    strtok(NULL, " ");
    char* dest_id = strtok(NULL, " ");

    //If this sensor is destination
    if (strcmp(dest_id, sensor_id) == 0)
    {
        printf("%s: Message from %s to %s successfully received.\n", sensor_id, origin_id, sensor_id);
        return;
    }

    //Get hop list
    int hop_len = atoi(strtok(NULL, " "));
    char** hop_list = (char**)calloc(hop_len, sizeof(char*));
    for (int i = 0; i < hop_len; i++)
    {
        hop_list[i] = strtok(NULL, " ");
    }

    //Send UPDATEPOSITION
    bzero(update, 200);
    sprintf(update, "UPDATEPOSITION %s %d %f %f", sensor_id, range, x_pos, y_pos);
    send(sockfd, update, strlen(update), 0);

    //Get list of reachable stations
    bzero(update, 200);
    while (update[0] != 'R')
    {
        bzero(update, 200);
        read(sockfd, update, 200);
    }
    get_reach_list(update, &reach_num, reach_list);

    //If destination is reachable
    for (int i = 0; i < reach_num; i++)
    {
        if (strcmp(reach_list[i].id, dest_id) == 0)
        {
            bzero(datamsg, 200);
            sprintf(datamsg, "DATAMESSAGE %s %s %s %d", origin_id, dest_id, dest_id, hop_len + 1);
            for (int i = 0; i < hop_len; i++)
            {
                strcat(datamsg, " ");
                strcat(datamsg, hop_list[i]);
            }
            strcat(datamsg, " ");
            strcat(datamsg, sensor_id);
            send(sockfd, datamsg, strlen(datamsg), 0);
            printf("%s: Message from %s to %s being forwarded through %s\n", sensor_id, origin_id, dest_id, sensor_id);
            return;
        }
    }

    int s_num;
    struct station* s_list = get_send_list(hop_list, hop_len, reach_list, reach_num, &s_num, 0.0, 0.0);

    // if no node can send
    if (s_num == 0)
    {
        printf("%s: Message from %s to %s could not be delivered.\n", sensor_id, origin_id, dest_id);
        return;
    }

    free(s_list);

    //Send where to get position of destination.
    char wheremsg[200];
    bzero(wheremsg, 200);
    sprintf(wheremsg, "WHERE %s", dest_id);
    send(sockfd, wheremsg, strlen(wheremsg), 0);
    bzero(wheremsg, 200);
    while (wheremsg[0] != 'T')
    {
        read(sockfd, wheremsg, 200);
    }
    strtok(wheremsg, " ");
    strtok(NULL, " ");
    float dest_x = atof(strtok(NULL, " "));
    float dest_y = atof(strtok(NULL, " "));

    s_list = get_send_list(hop_list, hop_len, reach_list, reach_num, &s_num, dest_x, dest_y);

    qsort(s_list, s_num, sizeof(struct station), cmpfunc);

    /*
    for (int i = 0; i < s_num; i++)
    {
        printf("%s %f\n", s_list[i].id, s_list[i].dist);
    }
    */

    bzero(datamsg, 200);
    sprintf(datamsg, "DATAMESSAGE %s %s %s %d", origin_id, s_list[0].id, dest_id, hop_len + 1);
    for (int i = 0; i < hop_len; i++)
    {
        strcat(datamsg, " ");
        strcat(datamsg, hop_list[i]);
    }
    strcat(datamsg, " ");
    strcat(datamsg, sensor_id);
    send(sockfd, datamsg, strlen(datamsg), 0);
    printf("%s: Message from %s to %s being forwarded through %s\n", sensor_id, origin_id, dest_id, sensor_id);
    return;
}

int main(int argc, char* argv[]){
    if (argc != 7)
    {
        fprintf(stderr, "invalid command line\n");
        exit(1);
    }

    struct addrinfo* results;
    char ip_addr[256];
    
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
    
    struct addrinfo* result = results;
    //struct addrinfo* temp = results;
    
    bzero(ip_addr, 256);
    getnameinfo(result -> ai_addr, result -> ai_addrlen, ip_addr, sizeof(ip_addr), NULL, 0, NI_NUMERICHOST);

    int sockfd;
    struct sockaddr_in servaddr;
    //int addrlen;
    int port = atoi(argv[2]);
    
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("socket create failed\n");
        return 0;
    }
    
    bzero(&servaddr, sizeof(servaddr)); 
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip_addr);

    sensor_id = argv[3];
    range = atoi(argv[4]);
    x_pos = atof(argv[5]);
    y_pos = atof(argv[6]);
    
    if(connect(sockfd, (struct sockaddr*)& servaddr, sizeof(servaddr)) < 0){
        printf("connect failed\n");
        return 0;
    }
    
    char* buffer = (char*)calloc(200,sizeof(char));
    char* buffer2 = (char*)calloc(200,sizeof(char));
    fd_set fds;
    char update[200] = {0};
    //Send UPDATEPOSITION
    sprintf(update, "UPDATEPOSITION %s %d %d %d", sensor_id, range, (int)x_pos, (int)y_pos);
    send(sockfd, update, strlen(update), 0);

    //Get list of station reachable
    bzero(update, 200);
    while (update[0] != 'R')
    {
        bzero(update, 200);
        read(sockfd, update, 200);
    }
    
    while(1){
        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        FD_SET(0, &fds);
        int fd_max = max(sockfd, 0);
        select(fd_max + 1, &fds, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &fds)) {
            bzero(buffer, 200);
            if (read(sockfd, buffer, 200) <= 0)
            {
                bzero(buffer2, 200);
                fgets(buffer2, 200, stdin);
                return 0;
            }
            do_data(sockfd, buffer);
            bzero(buffer, 200);
        }
        if (FD_ISSET(0, &fds))
        {
            bzero(buffer2, 200);
            fgets(buffer2, 200, stdin);
            if (buffer2[strlen(buffer2) - 1] == '\n')
            {
                buffer2[strlen(buffer2) - 1] = '\0';
            }
            if (buffer2[0] == 'M')
            {
                move(sockfd, buffer2);
            }
            if (buffer2[0] == 'S')
            {
                senddata(sockfd, buffer2);
            }
            if (buffer2[0] == 'Q')
            {
                break;
            }
            bzero(buffer2, 200);
        }
    }
    close(sockfd);
    return 0;
}
