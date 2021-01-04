#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define inputFER_LENGTH 1024
#define TOKEN_DELIMS " \t\r\n"

typedef struct shell_command_t
{
    char *argv[16];
    int stdin_fileno;
    int stdout_fileno;
    int stderr_fileno;
    int pipes[2];
    bool has_pipe;
    bool is_background;
    struct shell_command_t *next;
} command_t;

typedef struct child_pids
{
    pid_t idx;
    struct child_pids *next;
} child_pids_t;

child_pids_t *head = NULL;

void run_myshell();
void exit_myshell();
char *read_line();
char **parse_individual_commands(char *input);
char **tokenize(char *input);
void run_command(command_t *command_t);
void parse_command(char *input);
void insert_pid(pid_t pid);
void remove_pid(pid_t pid);
void sigchild(int signal);
int has_pipe(char *input);
void sigint(int signal);
void sigterm(int signal);
void default_args(command_t *ptr);

void sigchild(int signal)
{
    // does nothing
}

void sigint(int signal)
{
    child_pids_t *trav = head;
    while (trav != NULL)
    {
        if (kill(trav->idx, SIGTERM) < 0)
        {
            fprintf(stderr, "myshell: failed to stop child process!\n");
        }
        trav = trav->next;
    }
}

void sigterm(int signal)
{
    printf("myshell: exiting...\n");
    exit(signal);
}

void insert_pid(pid_t pid)
{
    child_pids_t *trav = (child_pids_t *)malloc(sizeof(child_pids_t));
    trav->next = head;
    trav->idx = pid;
    head = trav;
}

void remove_pid(pid_t pid)
{
    if (head == NULL)
    {
        exit(EXIT_FAILURE);
    }
    child_pids_t *trav = head;
    child_pids_t *prev = NULL;
    while ((trav != NULL) && (trav->idx != pid))
    {
        prev = trav;
        trav = trav->next;
    }
    if (trav != NULL)
    {
        if (prev == NULL)
        {
            head = NULL;
        }
        else
        {
            prev->next = trav->next;
        }
        free(trav);
    }
    else
    {
        fprintf(stderr, "myshell: process not found!\n");
        exit(EXIT_FAILURE);
    }
}

int has_pipe(char *input)
{
    const char *shell_tokens[] = {"&", "|", "<", ">", "1>", "2>", "&>"};
    int i = 0;
    while (i < 7 && strcmp(input, shell_tokens[i]))
        i++;
    if (i == 7)
        return -1;
    return i;
}

void exit_myshell()
{
    // TODO: kill child process, and exit...
    printf("\nExited...\n");
    exit(EXIT_SUCCESS);
}

void run_command(command_t *head)
{
    int true_stdin = dup(STDIN_FILENO);
    int true_stdout = dup(STDOUT_FILENO);
    int true_stderr = dup(STDERR_FILENO);
    pid_t pid;
    if (head == NULL)
    {
        fprintf(stderr, "myshell: syntax error!\n");
        return;
    }
    while (head != NULL)
    {
        if ((head->pipes[0] || head->pipes[1]) && head->is_background)
        {
            fprintf(stderr, "myshell: tried piping from background process!\n");
            exit(EXIT_FAILURE);
        }
        if ((pid = fork()) < 0)
        {
            fprintf(stderr, "myshell: failed to fork!\n");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0)
        {
            dup2(head->stdin_fileno, STDIN_FILENO);
            dup2(head->stdout_fileno, STDOUT_FILENO);
            dup2(head->stderr_fileno, STDERR_FILENO);
            if (head->pipes[0])
                close(head->stdin_fileno);
            if (head->pipes[1])
                close(head->stdout_fileno);
            if (execvp(head->argv[0], head->argv) == -1)
            {
                fprintf(stderr, "myshell: failed to execvp");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            insert_pid(pid);
            if (!head->is_background)
            {
                int stats;
                waitpid(pid, &stats, 0);
                if (!WIFEXITED(stats) || (WIFEXITED(stats) && (WEXITSTATUS(stats) != EXIT_SUCCESS)))
                {
                    fprintf(stderr, "myshell: error in child process!\n");
                    exit(EXIT_FAILURE);
                }
                remove_pid(pid);
            }
            if (head->pipes[0])
                close(head->stdin_fileno);
            if (head->pipes[1])
                close(head->stdout_fileno);
        }
        head = head->next;
    }
    dup2(true_stdin, STDIN_FILENO);
    dup2(true_stdout, STDOUT_FILENO);
    dup2(true_stderr, STDERR_FILENO);
    close(true_stdin);
    close(true_stdout);
    close(true_stderr);
}

void default_args(command_t *ptr)
{
    ptr->stdin_fileno = STDIN_FILENO;
    ptr->stdout_fileno = STDOUT_FILENO;
    ptr->stderr_fileno = STDERR_FILENO;
    ptr->is_background = 0;
    ptr->pipes[0] = 0;
    ptr->pipes[1] = 0;
    ptr->next = NULL;
}

void parse_command(char *input)
{
    command_t *head = (command_t *)malloc(sizeof(command_t));
    if (head == NULL)
    {
        fprintf(stderr, "myshell: allocation error!\n");
        exit(EXIT_FAILURE);
    }
    default_args(head);
    command_t *trav = head;
    command_t *prev = NULL;
    int i = 0;
    int c;
    int fd[2];
    char *arg;
    FILE *file;
    while ((arg = strtok_r(input, TOKEN_DELIMS, &input)))
    {
        if ((c = has_pipe(arg)) != -1)
        {
            if (trav->argv[0] == NULL)
            {
                if (prev == NULL)
                {
                    fprintf(stderr, "myshell: invalide syntax!\n");
                    return;
                }
                trav = prev;
            }
            command_t *node = malloc(sizeof(command_t));
            if (node == NULL)
            {
                fprintf(stderr, "myshell: allocation error!\n");
                exit(EXIT_FAILURE);
            }
            default_args(node);
            switch (c)
            {
            case 0: //"&"
                trav->is_background = 1;
                break;
            case 1: //"|"
                if ((pipe(fd)) == -1)
                {
                    fprintf(stderr, "myshell: failed to open pipe!\n");
                    exit(EXIT_FAILURE);
                }
                trav->stdout_fileno = fd[1];
                node->stdin_fileno = fd[0];
                node->pipes[0] = 1;
                trav->pipes[1] = 1;
                break;
            case 2: //"<" stdin
                if ((file = fopen(strtok_r(input, TOKEN_DELIMS, &input), "r")) == NULL)
                {
                    fprintf(stderr, "myshell: error for stdin file\n");
                    return;
                }
                trav->stdin_fileno = fileno(file);
                break;
            case 3: //">" stdout
            case 4: //"1>" stdout
                if ((file = fopen(strtok_r(input, TOKEN_DELIMS, &input), "a")) == NULL)
                {
                    fprintf(stderr, "myshell: error for stdout file");
                    return;
                }
                trav->stdout_fileno = fileno(file);
                break;
            case 5: //"2>" stderr
                if ((file = fopen(strtok_r(input, TOKEN_DELIMS, &input), "a")) == NULL)
                {
                    fprintf(stderr, "myshell: error for stderr file");
                    return;
                }
                trav->stderr_fileno = fileno(file);
                break;
            case 6: //"&>" stdout & stderr
                if ((file = fopen(strtok_r(input, TOKEN_DELIMS, &input), "a")) == NULL)
                {
                    fprintf(stderr, "myshell: error for stderr/stdout file");
                    return;
                }
                trav->stdout_fileno = fileno(file);
                trav->stderr_fileno = fileno(file);
                break;
            default:
                break;
            }
            i = 0;
            prev = trav;
            if (trav->argv[0] != NULL)
            {

                trav->next = node;
                trav = node;
            }
        }
        else
        {
            trav->argv[i] = malloc(64);
            strcpy(trav->argv[i], arg);
            i++;
        }
    }
    if (head->argv[0] == NULL)
    {
        return;
    }
    trav = head;
    prev = trav;
    while (trav->next != NULL)
    {
        prev = trav;
        trav = trav->next;
    }
    if (trav->argv[0] == NULL)
        prev->next = NULL;
    return run_command(head);
}

char *read_line()
{
    unsigned long inputfsize = inputFER_LENGTH;
    int i = 0;
    char *input = (char *)malloc(inputFER_LENGTH * sizeof(char *));
    if (!input)
    {
        fprintf(stderr, "myshell: Allocation error\n");
        exit(EXIT_FAILURE);
    }
    char token;
    while ((token = getc(stdin)) != '\n')
    {
        if (token != EOF)
            input[i++] = token;
        else
        {
            free(input);
            exit_myshell();
        }

        if (i >= inputfsize)
        {
            inputfsize += inputFER_LENGTH;
            input = realloc(input, inputfsize * sizeof(char *));
            if (!input)
            {
                fprintf(stderr, "myshell: Allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
    input[i] = '\0';
    return input;
}

char **parse_individual_commands(char *input)
{
    int i = 0;
    char **parsed = (char **)malloc(strlen(input) * sizeof(char *));
    char *token;
    if (!parsed)
    {
        fprintf(stderr, "myshell: Allocation error\n");
        exit(EXIT_FAILURE);
    }
    token = strtok(input, ";");
    while (token != NULL)
    {
        parsed[i++] = token;
        token = strtok(NULL, ";");
    }
    parsed[i] = NULL;
    return parsed;
}

void run_myshell()
{
    char *input = NULL;
    char **commands = NULL;
    while (true)
    {
        if (isatty(STDIN_FILENO))
        {
            printf("myshell> ");
            input = read_line();
            commands = parse_individual_commands(input);
            int i = 0;
            while (commands[i] != NULL)
            {
                parse_command(commands[i]);
                i++;
            }
        }
        free(input);
        free(commands);
    }
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint);
    signal(SIGTERM, sigterm);
    signal(SIGCHLD, sigchild);
    run_myshell();
    return EXIT_SUCCESS;
}