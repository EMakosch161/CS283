#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dshlib.h"
#include "rshlib.h"

 // exec_remote_cmd_loop(server_ip, port)
//remote shell client; sends/receives buffers and connects to server
int exec_remote_cmd_loop(char *address, int port)
{
    char *cmd_buff = malloc(RDSH_COMM_BUFF_SZ);
    char *rsp_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (!cmd_buff || !rsp_buff) {
        fprintf(stderr, "Memory allocation failed\n");
        return ERR_MEMORY;
    }

    int cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        perror("start_client");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    while (1)
    {
        printf("%s", SH_PROMPT);
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            printf("\n");
            break;
        }
        //remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        //send cmd including null terminator
        size_t send_len = strlen(cmd_buff) + 1;
        ssize_t bytes_sent = send(cli_socket, cmd_buff, send_len, 0);
        if (bytes_sent < 0 || (size_t)bytes_sent != send_len) {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }

        //if cmd is exit or stop, break after sending
        if (strcmp(cmd_buff, EXIT_CMD) == 0 || strcmp(cmd_buff, "stop-server") == 0)
            break;

        //receive until server finds EOF
        while (1) {
            ssize_t recv_bytes = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ, 0);
            if (recv_bytes < 0) {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            if (recv_bytes == 0) {
                //server closed
                break;
            }
            int is_eof = (rsp_buff[recv_bytes - 1] == RDSH_EOF_CHAR) ? 1 : 0;
            if (is_eof) {
                rsp_buff[recv_bytes - 1] = '\0';
            }
            printf("%.*s", (int)recv_bytes, rsp_buff);
            if (is_eof)
                break;
        }
    }

    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
}


 // start_client(server_ip, port)
 //    creates a socket and connects to the given server
int start_client(char *server_ip, int port)
{
    int cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_socket < 0) {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    if (connect(cli_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    return cli_socket;
}

 //helper for cleanup
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc)
{
    if (cli_socket > 0) {
        close(cli_socket);
    }
    free(cmd_buff);
    free(rsp_buff);
    return rc;
}