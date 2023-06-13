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
#include <dirent.h>
#include <netinet/in.h>

#define MAX_LEN 1024

void send_file_list(int client);
void send_file_content(int client, char* filename);

int main() {
    int client;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9000);

    if (bind(server, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server, 5) == -1) {
        perror("listen() failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        client = accept(server, (struct sockaddr *)&client_addr, &client_len);
        if (client == -1) {
            perror("accept() failed");
            exit(EXIT_FAILURE);
        }

        int pid = fork();
        if (pid < 0) {
            perror("fork() failed");
            exit(EXIT_FAILURE);
        } 
        else if (pid == 0) {
            close(server);
            send_file_list(client);
            char filename[MAX_LEN];
            memset(filename, 0, sizeof(filename));
            read(client, filename, sizeof(filename));

            filename[strcspn(filename, "\n")] = '\0';
            send_file_content(client, filename);

            close(client);
            exit(EXIT_SUCCESS);
        }
        else {
            close(client);
        }
    }
    close(server);

    return 0;
}

void send_file_list(int client) {
    DIR *dir;
    struct dirent *ent;

    dir = opendir(".");
    if (dir == NULL) {
        perror("Unable to open directory");
        char *error_msg = "ERRORNofilestodownload\r\n";
        write(client, error_msg, strlen(error_msg));
        close(client);
        exit(EXIT_FAILURE);
    }
    int file_count = 0;
    char file_list[MAX_LEN];
    memset(file_list, 0, sizeof(file_list));

    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_type == 8) {
            file_count++;
            strcat(file_list, ent->d_name);
            strcat(file_list, "\r\n");
        }
    }
    closedir(dir);

    if (file_count > 0) {
        char response[MAX_LEN];
        sprintf(response, "OK%d\r\n%s\r\n\r\n", file_count, file_list);
        write(client, response, strlen(response));
    } else {
        char *error_msg = "ERRORNofilestodownload\r\n";
        write(client, error_msg, strlen(error_msg));
        close(client);
        exit(EXIT_FAILURE);
    }
}

void send_file_content(int client, char* filename) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        char *error_msg = "ERRORFileNotFound\r\n";
        write(client, error_msg, strlen(error_msg));

        char response[MAX_LEN];
        memset(response, 0, sizeof(response));
        read(client, response, sizeof(response));

        response[strcspn(response, "\n")] = '\0';
        send_file_content(client, response);
    } else {
        fseek(file, 0L, SEEK_END);
        long file_size = ftell(file);
        rewind(file);
        char response[MAX_LEN];
        sprintf(response, "OK %ld\r\n", file_size);
        write(client, response, strlen(response));

        char buf[MAX_LEN];
        size_t bytes_read;

        while ((bytes_read = fread(buf, 1, sizeof(buf), file)) > 0) {
            write(client, buf, bytes_read);
        }
        fclose(file);
    }
}