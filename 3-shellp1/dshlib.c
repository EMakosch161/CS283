#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

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

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */

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