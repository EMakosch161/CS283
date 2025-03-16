#ifndef __RSH_LIB_H__
#define __RSH_LIB_H__

#include "dshlib.h"

// Common remote shell client and server constants and definitions

// Constants for communication
#define RDSH_DEF_PORT           1234        // Default port #
#define RDSH_DEF_SVR_INTFACE    "0.0.0.0"   // Default: bind to all interfaces
#define RDSH_DEF_CLI_CONNECT    "127.0.0.1" // Default client connects to localhost

// Constants for buffer sizes
#define RDSH_COMM_BUFF_SZ       (1024*64)   // 64K

// End of message delimiter (EOF)
static const char RDSH_EOF_CHAR = 0x04;

// rdsh specific error codes for functions
#define ERR_RDSH_COMMUNICATION  -50
#define ERR_RDSH_SERVER         -51
#define ERR_RDSH_CLIENT         -52
#define ERR_RDSH_CMD_EXEC       -53
#define WARN_RDSH_NOT_IMPL      -99

// Output message constants for server
#define CMD_ERR_RDSH_COMM   "rdsh-error: communications error\n"
#define CMD_ERR_RDSH_EXEC   "rdsh-error: command execution error\n"
#define CMD_ERR_RDSH_ITRNL  "rdsh-error: internal server error - %d\n"
#define CMD_ERR_RDSH_SEND   "rdsh-error: partial send.  Sent %d, expected to send %d\n"
#define RCMD_SERVER_EXITED  "server appeared to terminate - exiting\n"

// Output message constants for client
#define RCMD_MSG_CLIENT_EXITED  "client exited: getting next connection...\n"
#define RCMD_MSG_SVR_STOP_REQ   "client requested server to stop, stopping...\n"
#define RCMD_MSG_SVR_EXEC_REQ   "rdsh-exec:  %s\n"
#define RCMD_MSG_SVR_RC_CMD     "rdsh-exec:  rc = %d\n"

// Client prototypes for rsh_cli.c
int start_client(char *address, int port);
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc);
int exec_remote_cmd_loop(char *address, int port);

// Server prototypes for rsh_server.c
int start_server(char *ifaces, int port, int is_threaded);
int boot_server(char *ifaces, int port);
int stop_server(int svr_socket);
int send_message_eof(int cli_socket);
int send_message_string(int cli_socket, char *buff);
int process_cli_requests(int svr_socket);
int exec_client_requests(int cli_socket);
int rsh_execute_pipeline(int socket_fd, command_list_t *clist);

Built_In_Cmds rsh_match_command(const char *input);
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd);

#endif