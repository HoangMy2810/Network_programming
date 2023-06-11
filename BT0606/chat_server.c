#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h>

#define MAX_CLIENT 10
#define MAX_MSG_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char id[20];
    char name[50];
} client_t;

client_t clients[MAX_CLIENT];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr;

void *client_proc(void *param){
    client_t *client = (client_t *)param;
    char buf[MAX_MSG_LEN];
    while(1){
        char *msg = "Enter your \"client_id: client_name\": ";
        if (send(client->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
        }
        memset(buf, 0, MAX_MSG_LEN);
        int ret = recv(client->sockfd, buf, MAX_MSG_LEN, 0);
        if (ret < 0)
        {
            perror("recv() failed");
        }
        else if (ret == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd == clients[i].sockfd)
                {
                    pthread_mutex_lock(&clients_mutex);
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    if (client_count == 0)
                    {
                        printf("Waiting for clients on %s:%d...\n",
                               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    break;
                }
            }
            return NULL;
        }
        else
        {
            buf[strcspn(buf, "\n")] = 0;
            char id[MAX_MSG_LEN];
            char name[MAX_MSG_LEN];
            char temp[MAX_MSG_LEN];

            int ret = sscanf(buf, "%s %s %s", id, name, temp);
            if (ret == 2)
            {
                int len = strlen(id);
                if (id[len-1] != ':')
                {
                    char *msg = "Sai cu phap! Hay nhap lai! \n";
                    if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                    }
                    continue;
                }
                else
                {
                    id[len-1] = 0;
                    strcpy(client->id, id);
                    strcpy(client->name, name);
                    char *msg = "Dang nhap thanh cong!\n";
                    if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                    }

                    break;
                }
            }
            else{
                char *msg = "Sai cu phap! Hay nhap lai!\n";
                if (send(client->sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                }
                continue;
            }
        }
    }
    while(1){
        memset(buf, 0, MAX_MSG_LEN);
        int len = recv(client->sockfd, buf, MAX_MSG_LEN, 0);
        if (len < 0)
        {
            perror("recv() failed");
        }
        else if (len == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client->addr.sin_addr), ntohs(client->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd == clients[i].sockfd)
                {
                    pthread_mutex_lock(&clients_mutex);
                    clients[i] = clients[client_count - 1];
                    client_count--;
                    if (client_count == 0)
                    {
                        printf("Waiting for clients on %s:%d...\n",
                               inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
                    }
                    pthread_mutex_unlock(&clients_mutex);
                    break;
                }
            }
            break;
        }
        else{
            buf[strcspn(buf, "\n")] = 0;
            time_t now = time(NULL);
            struct tm *t = localtime(&now);
            char time_str[22];
            memset(time_str, 0, 22);
            strftime(time_str, MAX_MSG_LEN, "%Y/%m/%d %I:%M:%S%p", t);
            char msg_to_send[MAX_MSG_LEN + 58];
            sprintf(msg_to_send, "\n%s %s: %s\n\n", time_str, client->id, buf);
            for (int i = 0; i < client_count; i++)
            {
                if (client->sockfd != clients[i].sockfd)
                {
                    if (strcmp(clients[i].id, "") == 0)
                    {
                        continue;
                    }
                    if (send(clients[i].sockfd, msg_to_send, strlen(msg_to_send), 0) < 0)
                    {
                        perror("send() failed");
                    }
                }
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]){
    if(argc != 2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int server = socket(AF_INET, SOCK_DGRAM, 0);
    if(server == -1){
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if(bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr))){
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if(listen(server, MAX_CLIENT) == -1){
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }
    printf("Waiting for clients on %s:%d...\n",
           inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    while (1)
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);
        int client = accept(server, (struct sockaddr *)&client_addr, &client_len);
        if (client < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }

        if (client_count == MAX_CLIENT)
        {
            char *msg = "Too much connection.\n";
            if (send(client, msg, strlen(msg), 0) < 0)
            {
                perror("send() failed");
            }
            close(client);
            continue;
        }

        printf("Client from %s:%d connected\n",
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        pthread_mutex_lock(&clients_mutex);
        clients[client_count].sockfd = client;
        clients[client_count].addr = client_addr;
        strcpy(clients[client_count].name, "");
        strcpy(clients[client_count].id, "");
        client_count++;
        pthread_mutex_unlock(&clients_mutex);

        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, client_proc, (void *)&clients[client_count - 1]) != 0)
        {
            perror("pthread_create() failed");
            exit(EXIT_FAILURE);
        }
        pthread_detach(thread_id);
    }

    close(server);
    return 0;
}