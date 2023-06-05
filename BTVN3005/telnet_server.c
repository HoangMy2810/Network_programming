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

#define MAX_CLIENTS 10
#define MAX_LEN 1024

typedef struct client
{
    int sockfd;
    struct sockaddr_in addr;
    char username[MAX_LEN];
    char password[MAX_LEN];
} client_t;

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

void do_someth(int sockfd, char *command)
{
    char cmd[MAX_LEN];
    sprintf(cmd, "%s > out.txt", command);

    int status = system(cmd);
    if (status == 0)
    {
        FILE *fp = fopen("out.txt", "r");
        if (fp == NULL)
        {
            return;
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
        char *msg = "\nNhap lenh ban muon thuc hien: ";
        if (send(sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            return;
        }
        fclose(fp);
    }
    else
    {
        char *msg = "Khong tim thay lenh! Nhap lai: ";
        if (send(sockfd, msg, strlen(msg), 0) < 0)
        {
            perror("send() failed");
            return;
        }
    }
}

void signalHandler(){
    int stat;
    int pid = wait(&stat);
    if (pid > 0)
    {
        printf("Child %d terminated with exit status %d\n", pid, stat);
    }
    return;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, MAX_CLIENTS) < 0)
    {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, signalHandler);
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

            if (fork() == 0)
            {
                close(sockfd);
                client_t client_info;
                memset(&client_info, 0, sizeof(client_info));
                client_info.sockfd = client_sockfd;
                client_info.addr = client_addr;
                strcpy(client_info.username, "");
                strcpy(client_info.password, "");
                printf("New client connected.\n");

                char *msg = "Nhap \"username password\": ";
                if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                {
                    perror("send() failed");
                    continue;
                }
                char buf[1024];

            while (1)
            {
                memset(buf, 0, sizeof(buf));
                int bytes_received = recv(client_sockfd, buf, sizeof(buf), 0);
                if (bytes_received < 0)
                {
                    perror("recv() failed");
                    break;
                }
                else if (bytes_received == 0)
                {
                    printf("Client from %s:%d disconnected\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    break;
                }
                else
                {
                    buf[strcspn(buf, "\n")] = 0;
                    if (strcmp(client_info.username, "") == 0 && strcmp(client_info.password, "") == 0)
                    {
                        char username[32];
                        char password[32];
                        char temp[1024];
                        int ret = sscanf(buf, "%s %s %s", username, password, temp);
                        if (ret == 2)
                        {
                            int status = check_user(username, password);
                            if (status == 1)
                            {
                                strcpy(client_info.username, username);
                                strcpy(client_info.password, password);

                                char *msg = "Dang nhap thanh cong. Hay nhap lenh: ";
                                if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                            else if (status == 0)
                            {
                                char *msg = "Dang nhap ko thanh cong. Hay nhap lai ";
                                if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                            else
                            {
                                char *msg = "Khong tim thay database";
                                if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                                {
                                    perror("send() failed");
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            char *msg = "Sai cu phap. Hay nhap lai: ";
                            if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                        }
                    }
                    else
                    {
                        if (strcmp(buf, "quit") == 0 || strcmp(buf, "exit") == 0)
                        {
                            if (send(client_sockfd, msg, strlen(msg), 0) < 0)
                            {
                                perror("send() failed");
                                continue;
                            }
                            break;
                        }
                        else
                        {
                            do_someth(client_sockfd, buf);
                        }
                    }
                }
            }
            close(client_sockfd);
            exit(EXIT_SUCCESS);
        }
    }
    close(sockfd);

    return 0;
}
