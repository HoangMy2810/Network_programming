#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_LENGTH 1024
#define MAX_CLIENT 10
#define MAX_THREADS 8

void *client_proc(void *param)
{
    int server = *(int *)param;
    char buf[MAX_LENGTH];

    while (1)
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(server, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client < 0)
        {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }
        int ret = recv(client, buf, MAX_LENGTH, 0);
        if (ret < 0)
        {
            perror("recv() failed");
            continue;
        }
        else if (ret == 0)
        {
            close(client);
            continue;
        }
        else
        {
            buf[strcspn(buf, "\n")] = 0;
            printf("Received %d bytes from client %d: %s\n", ret, client, buf);
            char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html> \n";
            if (send(client, msg, strlen(msg), 0) < 0)
            {
                perror("send() failed");
                exit(EXIT_FAILURE);
            }
        }
        close(client);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if(server < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));
    if(bind(server, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0){
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }
    if(listen(server, MAX_CLIENT) == -1){
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }
    pthread_t thread_id;
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (pthread_create(&thread_id, NULL, client_proc, &server) != 0)
        {
            perror("pthread_create() failed");
            sched_yield();
        }
    }
    pthread_join(thread_id, NULL);
    getchar();
    close(server);
    return 0;
}