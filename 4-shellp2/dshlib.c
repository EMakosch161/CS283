#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include "dshlib.h"
//Eric Makosch -- assignment 4 shell.
#ifndef COMMAND_LIST_T_DEFINED
#define COMMAND_LIST_T_DEFINED

#define N_ARG_MAX    15   //max args for a command

typedef struct command {
    char exe[EXE_MAX];
    char args[ARG_MAX];
    int  argc;
    char *argv[N_ARG_MAX + 1];
} command_t;

typedef struct {
    int num;
    command_t commands[CMD_MAX];
} command_list_t;

#endif

// helpter for trimming command whitespace
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

int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    clist->num = 0;
    // Duplicate the entire input line so we can safely tokenize it.
    char *line_copy = strdup(cmd_line);
    if (line_copy == NULL)
    {
        perror("strdup");
        exit(EXIT_FAILURE);
    }
    // Trim the overall input.
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
        // Duplicate the individual command token.
        char *cmd_dup = strdup(cmd_trimmed);
        if (cmd_dup == NULL)
        {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        char *saveptr_word;
        char *word = strtok_r(cmd_dup, " \t", &saveptr_word);
        if (word == NULL)
        {
            free(cmd_dup);
            pipe_token = strtok_r(NULL, PIPE_STRING, &saveptr_pipe);
            continue;
        }
        if (strlen(word) >= EXE_MAX)
        {
            free(cmd_dup);
            free(line_copy);
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        strcpy(clist->commands[clist->num].exe, word);
        clist->commands[clist->num].args[0] = '\0';
        int first_arg = 1;
        while ((word = strtok_r(NULL, " \t", &saveptr_word)) != NULL)
        {
            size_t current_len = strlen(clist->commands[clist->num].args);
            size_t word_len = strlen(word);
            if (current_len + (first_arg ? 0 : 1) + word_len >= ARG_MAX)
            {
                free(cmd_dup);
                free(line_copy);
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            if (!first_arg)
                strcat(clist->commands[clist->num].args, " ");
            strcat(clist->commands[clist->num].args, word);
            first_arg = 0;
        }
        free(cmd_dup);
        clist->num++;
        pipe_token = strtok_r(NULL, PIPE_STRING, &saveptr_pipe);
    }
    free(line_copy);
    if (clist->num == 0)
        return WARN_NO_CMDS;
    return OK;
}

//implementation of P4 shell
//stores return code of last external cmd
int last_rc = 0;

//allocate and initailize cmd buff_t struct
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    cmd_buff->_cmd_buffer = NULL;
    return OK;
}

//free any memory allocated in cmd_buff_t
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer != NULL) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

//reset cmd buff_t struct to reuse
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


//parses CL into a cmd_buff_t, trims whitespace,
//collapses extra spaces, presveres outside quotes

int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    clear_cmd_buff(cmd_buff);
    char *trimmed = trim_whitespace(cmd_line);
    if (strlen(trimmed) == 0) {
        return WARN_NO_CMDS;
    }
    //allocate mem for buffer -- duplicate trimmed line
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
            // Cpature everything within quotes
            p++;
            start = p;
            while (*p && *p != '"')
                p++;
            if (*p == '"') {
                *p = '\0';  //terminate token
                p++;        //Skip closing quote
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
    return OK;
}

//check if ommand is built-in and returns its val
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0)
        return BI_CMD_EXIT;
    else if (strcmp(input, "cd") == 0)
        return BI_CMD_CD;
    else if (strcmp(input, "rc") == 0)
        return BI_RC;
    else if (strcmp(input, "dragon") == 0)
        return BI_CMD_DRAGON;
    else
        return BI_NOT_BI;
}

//executes built-in commands: "exit" quits, "cd" changes directory,
//"rc" prints last exit code, "dragon" is a placeholder.
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

        case BI_RC:
            printf("%d\n", last_rc);
            return BI_EXECUTED;

        case BI_CMD_DRAGON:
            /* Placeholder for extra credit dragon command */
            printf("Dragon not implemented.\n");
            return BI_EXECUTED;

        default:
            break;
    }
    return BI_NOT_BI;
}

//forks child to run external command through execvp the parent
//waits and saves the exit status in last_rc
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");

        return ERR_EXEC_CMD;
    }
    if (pid == 0) {
        execvp(cmd->argv[0], cmd->argv);
        //if execvp returns -> error
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
        //print error message if exit status =! 0
        if (exit_status != 0) {
            fprintf(stderr, CMD_ERR_EXECUTE);
        }

        return exit_status;
    }
}

//main shell loop that prints the prompt, reads input, parses it
//and executes commands.
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
        //Remove trailing newline
        input_buffer[strcspn(input_buffer, "\n")] = '\0';

        //nuild the command buffer from input
        int rc_build = build_cmd_buff(input_buffer, &cmd);
        if (rc_build != OK) {
            if (rc_build == WARN_NO_CMDS) {
                printf(CMD_WARN_NO_CMD);
                continue;
            } else if (rc_build == ERR_CMD_ARGS_BAD) {
                continue;
            }
        }

        //check for built in cmds
        if (cmd.argc > 0) {
            Built_In_Cmds bi = match_command(cmd.argv[0]);
            if (bi != BI_NOT_BI) {
                exec_built_in_cmd(&cmd);
                continue;
            }
        }

        //Otherwise execute external command using fork/exec
        exec_cmd(&cmd);
    }

    free_cmd_buff(&cmd);
    return OK;
}