#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct command_iter
{
    char **current;
    char **end;
}   command_iter;

typedef struct pipeline_iter
{
    char **current;
    char **end;
}   pipeline_iter;


typedef struct command 
{
    char **args;
    int fd_input;
    int fd_ouput;
}   command;

command command_init(char **args)
{
    return ((command){.args = args, .fd_input = STDIN_FILENO, .fd_ouput = STDOUT_FILENO});
}
#define FD_CLOSE (-42)

void command_fd_replace(command *cmd)
{
    if (cmd->fd_input != STDIN_FILENO)
    {
        if (dup2(cmd->fd_input, STDIN_FILENO) == -1)
            printf("Error: cmd %s dup2 fd input %i\n", cmd->args[0], cmd->fd_input);
        if (close(cmd->fd_input) == -1)
            printf("Error: cmd %s close fd input %i\n", cmd->args[0], cmd->fd_input);
    }
    if (cmd->fd_ouput != STDOUT_FILENO)
    {
        if (dup2(cmd->fd_ouput, STDOUT_FILENO) == -1)
            printf("Error: cmd %s dup2 fd ouput %i\n", cmd->args[0], cmd->fd_ouput);
        if (close(cmd->fd_ouput) == -1)
            printf("Error: cmd %s close fd ouput %i\n", cmd->args[0], cmd->fd_ouput);
    }
}

char **command_iter_look_ahead(command_iter *iter)
{
    if (iter->current < iter->end)
        return (iter->current);
    return (NULL);
}

char **command_iter_next(command_iter *iter)
{
    char **ret = iter->current;
    while (iter->current < iter->end)
    {
        iter->current++;
        if (iter->current[0] == NULL)
        {
            iter->current++;
            break;
        }
    }
    if (ret < iter->current)
        return (ret);
    return (NULL);
}

char **pipeline_iter_next(pipeline_iter *iter)
{
    char **ret = iter->current;
    while (iter->current < iter->end)
    {
        iter->current++;
        if (iter->current[0] == NULL)
        {
            iter->current++;
            break;
        }
    }
    if (ret < iter->current)
        return (ret);
    return (NULL);
}

#define PIPE_READ 0
#define PIPE_WRITE 1
void execute_cmd(command_iter *iter, char **envp, int previous_fd) //STDIN_AT_FIRST_CALL
{
    int pipes[2];
    pipes[PIPE_READ] = STDIN_FILENO;
    pipes[PIPE_WRITE] = STDOUT_FILENO;
    char **cmd_args = command_iter_next(iter);
    if (cmd_args == NULL)
        return ;
    command cmd = command_init(cmd_args);
    if (command_iter_look_ahead(iter))
    {
        pipe(pipes);
    }
    int pid = fork();
    if (pid == 0)
    { // child
        cmd.fd_input = previous_fd;
        cmd.fd_ouput = pipes[PIPE_WRITE];
        if (pipes[PIPE_READ] != STDIN_FILENO)
            close(pipes[PIPE_READ]);
        command_fd_replace(&cmd);
        execve(cmd.args[0], cmd.args, envp);
        exit(EXIT_FAILURE);
    }
    else
    {
        if (pipes[PIPE_WRITE] != STDOUT_FILENO)
            close(pipes[PIPE_WRITE]);
        if (previous_fd != STDIN_FILENO)
            close(previous_fd);
        previous_fd = pipes[PIPE_READ];
        
        execute_cmd(iter, envp, previous_fd);

        int waitstatus = 0;
        waitpid(pid, &waitstatus, 0);
    }
}

pipeline_iter pipeline_iter_init(int argc, char **argv)
{
    pipeline_iter iter = {.current = argv, .end = &argv[argc]};    
    
    for (size_t i = 0; argv[i]; i++)
    {
        if (*argv[i] == ';')
            argv[i] = NULL;
    }
    
    return (iter);
}


command_iter command_iter_init(size_t argc, char **argv)
{
    command_iter iter = {.current = argv, .end = &argv[argc]};    
    
    for (size_t i = 0; i < argc; i++)
    {
        if (*argv[i] == '|')
            argv[i] = NULL;
    }
    return (iter);
}

int execute_pipeline(pipeline_iter *pipel_iter, char **envp)
{
    char **pipeline_begin;
    while ((pipeline_begin = pipeline_iter_next(pipel_iter)))
    {
        size_t i = 0;
        for (i = 0; pipeline_begin[i]; i++);            
        command_iter cmd_itr = command_iter_init(i, pipeline_begin);
        execute_cmd(&cmd_itr, envp, STDIN_FILENO);
    }
    return (0);
}

int main(int argc, char **argv, char **envp)
{
    argv += 1;
    argc -= 1;
    if (argc == 0)
    {
        printf("no argument\n");
        exit(EXIT_SUCCESS);
    }
    pipeline_iter pipel_iter = pipeline_iter_init(argc, argv);
    execute_pipeline(&pipel_iter, envp);
}