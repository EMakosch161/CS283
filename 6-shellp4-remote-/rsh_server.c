#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "dshlib.h"
#include "rshlib.h"
#include <errno.h>


 // boot_server(ifaces, port)
int boot_server(char *ifaces, int port)
{
    int svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    int enable = 1;
    if (setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (bind(svr_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    if (listen(svr_socket, 20) < 0) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    return svr_socket;
}

 //close socket
int stop_server(int svr_socket)
{
    return close(svr_socket);
}

 // start_server
 //    Boots the server, processes client requests, and then stops the server.
int start_server(char *ifaces, int port, int is_threaded)
{
    (void)is_threaded;
    int svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        return svr_socket;
    }

    int rc = process_cli_requests(svr_socket);
    stop_server(svr_socket);
    return rc;
}


 // process_cli_requests(svr_socket)
 //  accepts client connections in a loop. For each accepted client socket,
 //    call exec_client_requests to handle client commands.
 //    if exec_client_requests returns ok_exit; loop breaks and server stops.

int process_cli_requests(int svr_socket)
{
    int rc = OK;
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int cli_socket = accept(svr_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (cli_socket < 0) {
            perror("accept");
            rc = ERR_RDSH_COMMUNICATION;
            break;
        }

        rc = exec_client_requests(cli_socket);
        close(cli_socket);
        if (rc == OK_EXIT)
            break;
    }
    return rc;
}

 // exec_client_requests(cli_socket)
 //reads command from client; uses loop to receive until null terminator found
 //then, checks for built in cmds, if none, builds cmd list using build func.and then executes
int exec_client_requests(int cli_socket)
{
    ssize_t io_size;
    command_list_t cmd_list;
    int rc;
    char *io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    while (1) {
        memset(io_buff, 0, RDSH_COMM_BUFF_SZ);
        int total_bytes = 0;
        //receive loop
        while (1) {
            io_size = recv(cli_socket, io_buff + total_bytes, RDSH_COMM_BUFF_SZ - total_bytes, 0);
            if (io_size < 0) {
                perror("recv");
                free(io_buff);
                return ERR_RDSH_COMMUNICATION;
            }
            if (io_size == 0) {
                //client diconnect
                free(io_buff);
                return OK;
            }
            total_bytes += io_size;
            if (memchr(io_buff, '\0', total_bytes) != NULL)
                break;
        }

        //process command received
        if (strcmp(io_buff, EXIT_CMD) == 0) {
            break;
        }
        if (strcmp(io_buff, "stop-server") == 0) {
            //stop server request feom cliemt
            free(io_buff);
            return OK_EXIT;
        }

        rc = build_cmd_list(io_buff, &cmd_list);
        if (rc != OK) {
            send_message_string(cli_socket, CMD_ERR_RDSH_EXEC);
            send_message_eof(cli_socket);
            continue;
        }


        int exec_rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        (void)exec_rc;

        free_cmd_list(&cmd_list);
        send_message_eof(cli_socket);
    }
    free(io_buff);
    return OK;
}

 // send_message_eof(cli_socket)
 //    sends the EOF marker to the client.
 //
int send_message_eof(int cli_socket)
{
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);
    if (sent_len != send_len) {
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}


 // send_message_string(cli_socket, buff)
 //    sends null-terminated string to the client; then sends
 //  EOF marker to show end of the message
int send_message_string(int cli_socket, char *buff)
{
    size_t len = strlen(buff);
    ssize_t sent = send(cli_socket, buff, len, 0);
    if (sent != (ssize_t)len)
        return ERR_RDSH_COMMUNICATION;
    return send_message_eof(cli_socket);
}

 // rsh_execute_pipeline(cli_sock, clist)

//executes pipleine of commands received from client.
int rsh_execute_pipeline(int cli_sock, command_list_t *clist)
{
    int num_cmds = clist->num;
    pid_t pids[CMD_MAX];
    int pipes[CMD_MAX-1][2];

    //creates pipes for intermediate processes
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return ERR_RDSH_CMD_EXEC;
        }
    }

    for (int i = 0; i < num_cmds; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return ERR_RDSH_CMD_EXEC;
        }
        if (pid == 0) {
            //child

               // duplicate cli_sock to stdin if not input redirection for first command
            if (i == 0) {
                char *infile = getenv("DSH_INFILE");
                if (infile == NULL) {
                    if (dup2(cli_sock, STDIN_FILENO) < 0) {
                        perror("dup2 cli_sock -> STDIN");
                        exit(errno);
                    }
                }
            } else {
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    perror("dup2 pipe -> STDIN");
                    exit(errno);
                }
            }

               //duplicate cli_sock to Stdout and stderr if not output redir for last cmd
            if (i == num_cmds - 1) {
                char *outfile = getenv("DSH_OUTFILE");
                if (outfile == NULL) {
                    if (dup2(cli_sock, STDOUT_FILENO) < 0) {
                        perror("dup2 cli_sock -> STDOUT");
                        exit(errno);
                    }
                    if (dup2(cli_sock, STDERR_FILENO) < 0) {
                        perror("dup2 cli_sock -> STDERR");
                        exit(errno);
                    }
                }
            } else {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 pipe -> STDOUT");
                    exit(errno);
                }
            }

            //close pipe descriptors in child
            for (int j = 0; j < num_cmds - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            //handle built in cmds
            Built_In_Cmds bi = rsh_match_command(clist->commands[i].argv[0]);
            if (bi != BI_NOT_BI) {
                rsh_built_in_cmd(&clist->commands[i]);
                exit(OK);
            }

            //execute external cmd
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(errno);
        } else {
            //parent
            pids[i] = pid;
        }
    }

    //close pipe desciptors
    for (int i = 0; i < num_cmds - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    //wait for child prcoesses to finish
    int status, exit_code = 0;
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], &status, 0);
        if (i == num_cmds - 1) {
            if (WIFEXITED(status))
                exit_code = WEXITSTATUS(status);
            else
                exit_code = -1;
        }
    }
    return exit_code;
}


 // rsh_match_command
 //   determines if the command is built-in.
Built_In_Cmds rsh_match_command(const char *input)
{
    if (strcmp(input, "exit") == 0)
        return BI_CMD_EXIT;
    if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    if (strcmp(input, "stop-server") == 0)
        return BI_CMD_STOP_SVR;
    if (strcmp(input, "rc") == 0)
        return BI_CMD_RC;
    return BI_NOT_BI;
}


 // rsh_built_in_cmd
 //    executes a built-in command for the remote shell

Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd)
{
    Built_In_Cmds type = rsh_match_command(cmd->argv[0]);
    switch (type)
    {
        case BI_CMD_EXIT:
            return BI_CMD_EXIT;
        case BI_CMD_STOP_SVR:
            return BI_CMD_STOP_SVR;
        case BI_CMD_RC:
            return BI_CMD_RC;
        case BI_CMD_CD:
            if (cmd->argc < 2) {
            } else if (cmd->argc == 2) {
                if (chdir(cmd->argv[1]) != 0)
                    perror("chdir");
            } else {
                fprintf(stderr, "cd: too many arguments\n");
            }
            return BI_EXECUTED;
        default:
            return BI_NOT_BI;
    }
}