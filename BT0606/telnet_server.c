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
    char username[MAX_LEN];
    char password[MAX_LEN];
} client_t;

client_t clients[MAX_CLIENT];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
struct sockaddr_in server_addr;
int check_user(char *username, char *password)
{
    FILE *fp = fopen("idname.txt", "r");
    if (fp == NULL)
    {
        return -1;
    }
    char buf[MAX_LEN];
    while (fgets(buf, MAX_LEN, fp) != NULL)
    {
        buf[strcspn(buf, "\n")] = 0;

        char user[MAX_LEN];
        char pass[MAX_LEN];
        sscanf(buf, "%s %s", user, pass);

        if (strcmp(username, user) == 0 && strcmp(password, pass) == 0)
        {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int do_someth(int sockfd, char *command)
{
    char cmd[MAX_LEN];
    sprintf(cmd, "%s > out.txt", command);

    int status = system(cmd);
    if (status == 0)
    {
        FILE *fp = fopen("out.txt", "r");
        if (fp == NULL)
        {
            perror("fopen() failed");
            return 1;
        }
        char buf[MAX_LEN];
        while (fgets(buf, MAX_LEN, fp) != NULL)
        {
            if (send(sockfd, buf, strlen(buf), 0) < 0)
            {
                perror("send() failed");
                continue;
            }
        }
        
        fclose(fp);
        remove("out.txt");
    }
   return status;
}

void *client_proc(void *param)
{
    client_t *client_info = (client_t *)param;
    char buf[MAX_LEN];

    while (1)
    {
        char *msg = "Nhap vao \"username password\": ";
        if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
        }
        memset(buf, 0, sizeof(buf));
        int ret = recv(client_info->sockfd, buf, sizeof(buf), 0);
        if (ret < 0)
        {
            perror("recv() failed");
            return NULL;
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
                    break;
                }
            }
            return NULL;
        }
        else
        {
            buf[strcspn(buf, "\n")] = 0;

            char username[MAX_LEN];
            char password[MAX_LEN];
            char temp[MAX_LEN];
            int ret = sscanf(buf, "%s %s %s", username, password, temp);
            if (ret == 2)
            {
                int status = check_user(username, password);
                if (status == 1)
                {
                    strcpy(client_info->username, username);
                    strcpy(client_info->password, password);

                    char *msg = "Dang nhap thanh cong! Hay nhap lenh: \n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    break;
                }
                else if (status == 0)
                {
                    char *msg = "Dang nhap ko thanh cong! Nhap lai \n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    continue;
                }
                else
                {
                    char *msg = "Server error\n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    continue;
                }
            }
            else
            {
                char *msg = "Sai cu phap! Hay nhap lai\n";
                if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
                continue;
            }
        }
    }
    while (1)
    {
        char *msg = "Nhap lenh: ";
        if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            continue;
        }
        memset(buf, 0, sizeof(BUFSIZ));
        int ret = recv(client_info->sockfd, buf, sizeof(buf), 0);
        if (ret < 0)
        {
            perror("recv() failed");
            return NULL;
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
                        break;
                    }
                }
                break;
            }
            else
            {
                if (do_someth(client_info->sockfd, buf) != 0)
                {
                    char *msg = "Sai cu phap! Hay nhap lai\n";
                    if (send(client_info->sockfd, msg, strlen(msg), 0) < 0)
                    {
                        perror("send() failed");
                        continue;
                    }
                    continue;
                }
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

    // signal(SIGCHLD, signalHandler);
    // client_t clients[MAX_CLIENTS];
    // int num_users = 0;
    // fd_set readfds;

    while (1)
    {
        // FD_ZERO(&readfds);
        // FD_SET(sockfd, &readfds);
        printf("Doi user ket noi....");
        // if (num_users == 0)
        // {
        //     printf("Doi user ket noi....");
        // }
        // else
        // {
        //     for (int i = 0; i < num_users; i++)
        //     {
        //         FD_SET(clients[i].sockfd, &readfds);
        //     }
        // }

        // if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0)
        // {
        //     perror("select() failed");
        //     continue;
        // }

        // if (FD_ISSET(sockfd, &readfds))
        // {
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
            strcpy(clients[client_count].username, "");
            strcpy(clients[client_count].password, "");
            client_count++;
            pthread_mutex_unlock(&clients_mutex);

            pthread_t thread_id;
            pthread_create(&thread_id, NULL, client_proc, (void *)&clients[client_count - 1]);
            pthread_detach(thread_id);
    }
    close(sockfd);
    return 0;
}
