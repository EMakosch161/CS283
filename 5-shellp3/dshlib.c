#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>

#include "dshlib.h"

#ifndef CMD_ERR_EXECUTE
#define CMD_ERR_EXECUTE "error: execution failure\n"
#endif


//Eric Makosch shell part 5 CS283


//no op if arg tokenized already
static void token_argv_for_cmd(cmd_buff_t *cmd) {
    (void)cmd;  /* Prevent unused parameter warning */
}

/* helpter for trimming command whitespace */
static char *trim_whitespace(char *str)
{
    while (isspace((unsigned char)*str))
        str++;
    if (*str == '\0')
        return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
    {
        *end = '\0';
        end--;
    }
    return str;
}

// build cmd list : split cmd_line and pipestring, building cmd_buff_t
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    clist->num = 0;
    //initalize cmd buffers
    for (int i = 0; i < CMD_MAX; i++) {
        clist->commands[i]._cmd_buffer = NULL;
        clist->commands[i].argc = 0;
        for (int j = 0; j < CMD_ARGV_MAX; j++) {
            clist->commands[i].argv[j] = NULL;
        }
    }
    //duplicate inp line for tokenization
    char *line_copy = strdup(cmd_line);
    if (line_copy == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    //trim inp
    char *trimmed_line = trim_whitespace(line_copy);
    if (strlen(trimmed_line) == 0)
    {
        free(line_copy);
        return WARN_NO_CMDS;
    }

    char *saveptr_pipe;
    char *pipe_token = strtok_r(trimmed_line, PIPE_STRING, &saveptr_pipe);
    while (pipe_token != NULL)
    {
        char *cmd_trimmed = trim_whitespace(pipe_token);
        if (strlen(cmd_trimmed) == 0)
        {
            pipe_token = strtok_r(NULL, PIPE_STRING, &saveptr_pipe);
            continue;
        }
        if (clist->num >= CMD_MAX)
        {
            free(line_copy);
            return ERR_TOO_MANY_COMMANDS;
        }
        //build cmd buffer for each token
        int rc = build_cmd_buff(cmd_trimmed, &clist->commands[clist->num]);
        if (rc != OK)
        {
            free(line_copy);
            return rc;
        }
        clist->num++;
        pipe_token = strtok_r(NULL, PIPE_STRING, &saveptr_pipe);
    }
    free(line_copy);
    if (clist->num == 0)
        return WARN_NO_CMDS;
    return OK;
}

// stores return code of last external cmd
int last_rc = 0;

// allocate and initailize cmd buff_t struct
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->_cmd_buffer = NULL;
    return OK;
}

//free memory in cmd_buff
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

//rest cmd buff for reuse
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

/* parses CL into a cmd_buff_t, trims whitespace,
   collapses extra spaces, presveres outside quotes */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    clear_cmd_buff(cmd_buff);
    char *trimmed = trim_whitespace(cmd_line);
    if (strlen(trimmed) == 0) {
        return WARN_NO_CMDS;
    }
    // allocate mem for buffer -- duplicate trimmed line
    cmd_buff->_cmd_buffer = strdup(trimmed);
    if (cmd_buff->_cmd_buffer == NULL) {
        perror("strdup");
        return ERR_MEMORY;
    }

    int argc = 0;
    char *p = cmd_buff->_cmd_buffer;
    while (*p != '\0') {
        //skip leading space
        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p == '\0')
            break;

        char *start;
        if (*p == '"') {
            //Cpature everything within quotes
            p++;
            start = p;
            while (*p && *p != '"')
                p++;
            if (*p == '"') {
                *p = '\0';  // terminate token
                p++;        // Skip closing quote
            }
        } else {
            start = p;
            while (*p && !isspace((unsigned char)*p))
                p++;
            if (*p) {
                *p = '\0';
                p++;
            }
        }

        if (argc < CMD_ARGV_MAX) {
            cmd_buff->argv[argc] = start;
            argc++;
        } else {
            fprintf(stderr, "error: too many arguments\n");
            return ERR_CMD_ARGS_BAD;
        }
    }
    cmd_buff->argc = argc;

    //A5 implementation
    {
        // new vars for redirection: input redir, output redir and append flag.
        char *new_argv[CMD_ARGV_MAX];
        int new_argc = 0;
        char *infile = NULL;
        char *outfile = NULL;
        int append_flag = 0;
        for (int i = 0; i < cmd_buff->argc; i++) {
            if (strcmp(cmd_buff->argv[i], "<") == 0) {
                if (i + 1 < cmd_buff->argc) {
                    infile = cmd_buff->argv[i+1];
                    i++; /* skip filename */
                }
            } else if (strcmp(cmd_buff->argv[i], ">") == 0) {
                if (i + 1 < cmd_buff->argc) {
                    outfile = cmd_buff->argv[i+1];
                    append_flag = 0;
                    i++;
                }
            } else if (strcmp(cmd_buff->argv[i], ">>") == 0) {
                if (i + 1 < cmd_buff->argc) {
                    outfile = cmd_buff->argv[i+1];
                    append_flag = 1;
                    i++;
                }
            } else {
                new_argv[new_argc++] = cmd_buff->argv[i];
            }
        }
        // update argv and argc with redirection tokens removed
        for (int i = 0; i < new_argc; i++) {
            cmd_buff->argv[i] = new_argv[i];
        }
        cmd_buff->argc = new_argc;
        //sore redir info in environment vars for exec_cmd to pick them up
        if (infile != NULL) {
            setenv("DSH_INFILE", infile, 1);
        } else {
            unsetenv("DSH_INFILE");
        }
        if (outfile != NULL) {
            setenv("DSH_OUTFILE", outfile, 1);
            char flag[2];
            snprintf(flag, sizeof(flag), "%d", append_flag);
            setenv("DSH_OUTAPPEND", flag, 1);
        } else {
            unsetenv("DSH_OUTFILE");
            unsetenv("DSH_OUTAPPEND");
        }
    }

    return OK;
}

//check if command is built in and return val
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0)
        return BI_CMD_EXIT;
    else if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    else if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    else
        return BI_NOT_BI;
}

   //execs built in cmds
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds type = match_command(cmd->argv[0]);
    switch (type) {
        case BI_CMD_EXIT:
            exit(0);
            break;
        case BI_CMD_CD:
            if (cmd->argc < 2) {
                //no provided dir -> do nothing
            } else if (cmd->argc == 2) {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("chdir");
                }
            } else {
                fprintf(stderr, "cd: too many arguments\n");
            }
            return BI_EXECUTED;
        case BI_CMD_DRAGON:
            printf("Dragon not implemented.\n");
            return BI_EXECUTED;
        default:
            break;
    }
    return BI_NOT_BI;
}

/* forks child to run external command through execvp the parent
   waits and saves the exit status in last_rc */
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return ERR_EXEC_CMD;
    }
    if (pid == 0) {
        {
            char *infile = getenv("DSH_INFILE");
            char *outfile = getenv("DSH_OUTFILE");
            int append_flag = 0;
            char *flag = getenv("DSH_OUTAPPEND");
            if (flag != NULL) {
                append_flag = atoi(flag);
            }
            if (infile != NULL) {
                int fd_in = open(infile, O_RDONLY);
                if (fd_in < 0) {
                    perror("open infile");
                    exit(errno);
                }
                if (dup2(fd_in, STDIN_FILENO) == -1) {
                    perror("dup2 infile");
                    exit(errno);
                }
                close(fd_in);
            }
            if (outfile != NULL) {
                int fd_out;
                if (append_flag) {
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }
                if (fd_out < 0) {
                    perror("open outfile");
                    exit(errno);
                }
                if (dup2(fd_out, STDOUT_FILENO) == -1) {
                    perror("dup2 outfile");
                    exit(errno);
                }
                close(fd_out);
            }
        }

        execvp(cmd->argv[0], cmd->argv);
        //return error if execvp returns
        perror("execvp");
        exit(errno);
    } else {
        // parent process
        int status;
        waitpid(pid, &status, 0);
        int exit_status = 0;
        if (WIFEXITED(status)) {
            exit_status = WEXITSTATUS(status);
        }
        last_rc = exit_status;
        // print error message if exit status != 0
        if (exit_status != 0) {
            fprintf(stderr, CMD_ERR_EXECUTE);
        }
        return exit_status;
    }
}

//func: execs pipeline of external cmds with redir
int execute_pipeline(command_list_t *clist) {
    int num_cmds = clist->num;
    pid_t pids[CMD_MAX];
    int prev_pipe_fd[2] = { -1, -1 };

    //retrive redir info from environment vars
    char *infile = getenv("DSH_INFILE");
    char *outfile = getenv("DSH_OUTFILE");
    int append_flag = 0;
    char *flag = getenv("DSH_OUTAPPEND");
    if (flag != NULL) {
        append_flag = atoi(flag);
    }

    for (int i = 0; i < num_cmds; i++) {
        //call to helper (no op)
        token_argv_for_cmd(&clist->commands[i]);

        int pipe_fd[2];
        if (i < num_cmds - 1) {
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                return ERR_EXEC_CMD;
            }
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        }
        if (pid == 0) {
            //in child process
            //if first command and input redirection is specified
            if (i == 0 && infile != NULL) {
                int fd_in = open(infile, O_RDONLY);
                if (fd_in < 0) {
                    perror("open infile");
                    exit(errno);
                }
                if (dup2(fd_in, STDIN_FILENO) == -1) {
                    perror("dup2 infile");
                    exit(errno);
                }
                close(fd_in);
            }
            //if not first cmd, redir STDIN to previous pipes read end
            if (i > 0) {
                if (dup2(prev_pipe_fd[0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(errno);
                }
            }
            //if last command and outpit redir is specified
            if (i == num_cmds - 1 && outfile != NULL) {
                int fd_out;
                if (append_flag) {
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                } else {
                    fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                }
                if (fd_out < 0) {
                    perror("open outfile");
                    exit(errno);
                }
                if (dup2(fd_out, STDOUT_FILENO) == -1) {
                    perror("dup2 outfile");
                    exit(errno);
                }
                close(fd_out);
            }
            //if not last command, redir stdout to current pipes write end
            if (i < num_cmds - 1) {
                if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(errno);
                }
            }
            if (i > 0) {
                close(prev_pipe_fd[0]);
                close(prev_pipe_fd[1]);
            }
            if (i < num_cmds - 1) {
                close(pipe_fd[0]);
                close(pipe_fd[1]);
            }
            //xecute the external command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            //if execvp returns, ->error
            perror("execvp");
            exit(errno);
        } else {
            //parent process: record child PID
            pids[i] = pid;
            if (i > 0) {
                //close previous pipe file descriptors
                close(prev_pipe_fd[0]);
                close(prev_pipe_fd[1]);
            }
            if (i < num_cmds - 1) {
                prev_pipe_fd[0] = pipe_fd[0];
                prev_pipe_fd[1] = pipe_fd[1];
            }
        }
    }
    //close remaining pipe descriptors
    if (num_cmds > 1) {
        close(prev_pipe_fd[0]);
        close(prev_pipe_fd[1]);
    }
    //wait for child proceses to finish
    int status;
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], &status, 0);
        if (i == num_cmds - 1) {
            if (WIFEXITED(status))
                last_rc = WEXITSTATUS(status);
            else
                last_rc = -1;
        }
    }
    if (last_rc != 0) {
        fprintf(stderr, CMD_ERR_EXECUTE);
    }
    return last_rc;
}

//func: extract redir tokens from inp and remove
void extract_redir(char *line, char **infile, char **outfile, int *append_flag) {
    char new_line[SH_CMD_MAX + 1];
    new_line[0] = '\0';
    *infile = NULL;
    *outfile = NULL;
    *append_flag = 0;

    char *token;
    char *saveptr;
    token = strtok_r(line, " \t", &saveptr);
    while (token != NULL) {
         if (strcmp(token, "<") == 0) {
             token = strtok_r(NULL, " \t", &saveptr);
             if (token) {
                 *infile = strdup(token);
             }
         } else if (strcmp(token, ">") == 0) {
             token = strtok_r(NULL, " \t", &saveptr);
             if (token) {
                 *outfile = strdup(token);
                 *append_flag = 0;
             }
         } else if (strcmp(token, ">>") == 0) {
             token = strtok_r(NULL, " \t", &saveptr);
             if (token) {
                 *outfile = strdup(token);
                 *append_flag = 1;
             }
         } else {
             if (strlen(new_line) > 0) {
                 strcat(new_line, " ");
             }
             strcat(new_line, token);
         }
         token = strtok_r(NULL, " \t", &saveptr);
    }
    strcpy(line, new_line);
}

//func: free all cmd buffs in list
int free_cmd_list(command_list_t *cmd_lst) {
    for (int i = 0; i < cmd_lst->num; i++) {
        if (cmd_lst->commands[i]._cmd_buffer) {
            free(cmd_lst->commands[i]._cmd_buffer);
            cmd_lst->commands[i]._cmd_buffer = NULL;
        }
    }
    cmd_lst->num = 0;
    return OK;
}


//main shell loop (prints prompt, reads input, parses)
int exec_local_cmd_loop()
{
    char input_buffer[SH_CMD_MAX + 1];
    cmd_buff_t cmd;
    alloc_cmd_buff(&cmd);
    last_rc = 0;

    while (1) {
        printf("%s", SH_PROMPT);
        if (fgets(input_buffer, sizeof(input_buffer), stdin) == NULL) {
            printf("\n");
            break;
        }
        //remove trailing \n
        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        char *redir_in = NULL;
        char *redir_out = NULL;
        int redir_append = 0;
        //make copy of inp buff sinc redir will modify
        char input_copy[SH_CMD_MAX + 1];
        strcpy(input_copy, input_buffer);
        extract_redir(input_copy, &redir_in, &redir_out, &redir_append);
        //copy modded line back to inpbuffer for processing
        strcpy(input_buffer, input_copy);

        //build cmd list
        command_list_t clist;
        int rc_list = build_cmd_list(input_buffer, &clist);
        if (rc_list != OK) {
            if (rc_list == WARN_NO_CMDS) {
                printf(CMD_WARN_NO_CMD);
            } else if (rc_list == ERR_TOO_MANY_COMMANDS) {
                fprintf(stderr, CMD_ERR_PIPE_LIMIT, CMD_MAX);
            }
            continue;
        }

        //if one command and its builtin, exec without fork
        if (clist.num == 1) {
            Built_In_Cmds bi = match_command(clist.commands[0].argv[0]);
            if (bi != BI_NOT_BI) {
                //temp cmd string combining args
                char temp_cmd[SH_CMD_MAX];
                //rebuild cmd link from saved buff
                strcpy(temp_cmd, clist.commands[0]._cmd_buffer);
                clear_cmd_buff(&cmd);
                int rc_build = build_cmd_buff(temp_cmd, &cmd);
                if (rc_build != OK) {
                    if (rc_build == WARN_NO_CMDS)
                        printf(CMD_WARN_NO_CMD);
                    free_cmd_list(&clist);
                    continue;
                }
                exec_built_in_cmd(&cmd);
                free_cmd_buff(&cmd);
                free_cmd_list(&clist);
                if (redir_in) free(redir_in);
                if (redir_out) free(redir_out);
                continue;
            }
        }

        //otherwise: exec external cmds
        //1 commands
        if (clist.num == 1) {
            if (redir_in != NULL || redir_out != NULL) {
                if (redir_in != NULL) {
                    setenv("DSH_INFILE", redir_in, 1);
                } else {
                    unsetenv("DSH_INFILE");
                }
                if (redir_out != NULL) {
                    setenv("DSH_OUTFILE", redir_out, 1);
                    char flag[2];
                    snprintf(flag, sizeof(flag), "%d", redir_append);
                    setenv("DSH_OUTAPPEND", flag, 1);
                } else {
                    unsetenv("DSH_OUTFILE");
                    unsetenv("DSH_OUTAPPEND");
                }
            }
            char temp_cmd[SH_CMD_MAX];
            strcpy(temp_cmd, clist.commands[0]._cmd_buffer);
            clear_cmd_buff(&cmd);
            int rc_build = build_cmd_buff(temp_cmd, &cmd);
            if (rc_build != OK) {
                if (rc_build == WARN_NO_CMDS)
                    printf(CMD_WARN_NO_CMD);
                free_cmd_list(&clist);
                continue;
            }
            exec_cmd(&cmd);
        } else {
            //multiple commands: execute as pipeline. */
            if (redir_in != NULL) {
                setenv("DSH_INFILE", redir_in, 1);
            } else {
                unsetenv("DSH_INFILE");
            }
            if (redir_out != NULL) {
                setenv("DSH_OUTFILE", redir_out, 1);
                char flag[2];
                snprintf(flag, sizeof(flag), "%d", redir_append);
                setenv("DSH_OUTAPPEND", flag, 1);
            } else {
                unsetenv("DSH_OUTFILE");
                unsetenv("DSH_OUTAPPEND");
            }
            execute_pipeline(&clist);
        }
        free_cmd_list(&clist);
        if (redir_in) free(redir_in);
        if (redir_out) free(redir_out);
    }

    free_cmd_buff(&cmd);
    return OK;
}