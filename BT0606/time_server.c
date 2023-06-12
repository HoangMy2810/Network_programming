#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAX_CLIENT 10
#define MAX_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
} client_t;

client_t clients[MAX_CLIENT];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr;

void format_time(char buf[], const char *format)
{
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    if (strcmp(format, "dd/mm/yyyy") == 0)
    {
        strftime(buf, MAX_LEN, "%d/%m/%Y\n", timeinfo);
    }
    else if (strcmp(format, "dd/mm/yy") == 0)
    {
        strftime(buf, MAX_LEN, "%d/%m/%y\n", timeinfo);
    }
    else if (strcmp(format, "mm/dd/yyyy") == 0)
    {
        strftime(buf, MAX_LEN, "%m/%d/%Y\n", timeinfo);
    }
    else if (strcmp(format, "mm/dd/yy") == 0)
    {
        strftime(buf, MAX_LEN, "%m/%d/%y\n", timeinfo);
    }
    else
    {
        strcpy(buf, "Invalid format\n");
    }
}

void *client_proc(void *param)
{
    client_t *client_info = (client_t *)param;
    while (1)
    {
        char *msg = "Nhap lenh get time:";
        if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            break;
        }
        char buf[MAX_LEN];
        memset(buf, 0, sizeof(buf));
        int ret = recv(client_info->sockfd, buf, sizeof(buf), 0);
        if (ret < 0)
        {
            perror("recv() failed");
            break;
        }
        else if (ret == 0)
        {
            printf("Client from %s:%d disconnected\n",
                   inet_ntoa(client_info->addr.sin_addr),
                   ntohs(client_info->addr.sin_port));
            for (int i = 0; i < client_count; i++)
            {
                if (client_info->sockfd == clients[i].sockfd)
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
                    close(client_info->sockfd);
                    break;
                }
            }
            break;
        }
        else
        {
            buf[strcspn(buf, "\n")] = 0;
            if (strcmp(buf, "quit") == 0 || strcmp(buf, "exit") == 0)
            {
                printf("Client from %s:%d disconnected\n",
                       inet_ntoa(client_info->addr.sin_addr),
                       ntohs(client_info->addr.sin_port));
                for (int i = 0; i < client_count; i++)
                {
                    if (client_info->sockfd == clients[i].sockfd)
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
                        close(client_info->sockfd);
                        break;
                    }
                }
                break;
            }
            char cmd[MAX_LEN];
            char format[MAX_LEN];
            char temp[MAX_LEN];
            int ret = sscanf(buf, "%s %s %s", cmd, format, temp);
            if (ret == 2)
            {
                if (strcmp(cmd, "GET_TIME") == 0)
                {
                    memset(buf, 0, MAX_LEN);
                    format_time(buf, format);
                    if (send(client_info->sockfd, buf, strlen(buf), 0) < 0)
                    {
                        perror("send() failed");
                        break;
                    }
                }
                else
                {
                    char *msg = "Invalid command\n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        break;
                    }
                    continue;
                }
            }
            else
            {
                char *msg = "Invalid command\n";
                if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    break;
                }
                continue;
            }
        }
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

    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, MAX_CLIENT) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }
    printf("Doi user ket noi....");
    while (1)
    {
        struct sockaddr_in client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sockfd = accept(sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sockfd < 0)
            {
                perror("accept() failed");
                exit(EXIT_FAILURE);
            }

        printf("New client connected from %s:%d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port));
        pthread_mutex_lock(&clients_mutex);
        clients[client_count].sockfd = client_sockfd;
        clients[client_count].addr = client_addr;
        client_count++;
        pthread_mutex_unlock(&clients_mutex);
        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_proc, (void *)&clients[client_count - 1]);
        pthread_detach(thread_id);
    }
    close(sockfd);
    return 0;
}
