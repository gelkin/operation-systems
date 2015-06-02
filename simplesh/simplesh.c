// TODO:
// not to read as much as possible, but read a string
// add check to buf_fill (mb create new buf_fill') if we had
// '\n' at the end of just read part of bytes -> if yes ->
// return what we got (it will work because read(...) returns
// its result as soon as it meets '\n'
#include "bufio.h"
#include "helpers.h"

#define BUF_SIZE 4096

size_t number_of_parts(char* line, ssize_t size, char delim)
{
    size_t number_of_parts = 1;
    int last_found_delim = -1;
    for (int i = 0; i < size; ++i)
    {
        // ignore delim at the end
        if (line[i] == delim && i != (size - 1))
        {
            // ignore delim at the begining and double delim
            if (last_found_delim != (i - 1))
            {
                ++number_of_parts;
            }
            last_found_delim = i;    
        }
    }
    return number_of_parts;
}

// split in parts by delimiter. 'parts' and 'parts_size' are returned
ssize_t split(char* line, size_t size, char delim, char** parts)
{
    ssize_t parts_size = number_of_parts(line, size, delim);
    parts = (char **) malloc((parts_size + 1) * sizeof(char *));
    parts[parts_size] = NULL;

    size_t one_part_size = 0;
    size_t part_number = 0;
    int last_found_delim = -1;
    for (int i = 0; i < size; ++i)
    {
        // ignore delim at the end
        if (line[i] == delim && i != (size - 1))
        {
            // ignore delim at the begining and double delim
            if (last_found_delim != (i - 1))
            {
                char* one_part = (char*) malloc((one_part_size + 1) * sizeof(char));
                if (one_part == NULL)
                {
                    perror("malloc");
                    return -1;
                }
                memcpy(one_part, line + (i - one_part_size), one_part_size);
                one_part[one_part_size] = '\0';

                parts[part_number] = one_part;
                ++part_number;
                one_part_size = 0;
            }
            last_found_delim = i;
        }
        else
        {
            ++one_part_size;
        }
    }

    char* one_part = (char*) malloc((one_part_size + 1) * sizeof(char));
    if (one_part == NULL)
    {
        perror("malloc");
        return -1;
    }
    memcpy(one_part, line + (i - one_part_size), one_part_size);
    one_part[one_part_size] = '\0';

    parts[part_number] = one_part;

    return parts_size;
}

ssize_t get_execargs(char* line, size_t size, struct execargs_t** programs)
{
    char** splitted_by_pipe; 
    ssize_t programs_size = split(line, size, '|', splitted_by_pipe);
    if (programs_size == -1)
    {
        perror("split");
        return -1;
    }
    programs[programs_size];
    
    for (int i = 0; i < programs_size; ++i)
    {
        char** current_program;
        ssize_t current_program_size = split(splitted_by_pipe[i],
                                            strlen(splitted_by_pipe[i]),
                                            ' ',
                                            current_program);
        if (current_program_size == -1)
        {
            perror("split");
            return -1;
        }

        struct execargs_t* program = (struct execargs_t*) malloc(sizeof(struct execargs_t));
        if (program == NULL)
        {
            return -1;
        }
        program->file = current_program[0];
        program->argv = current_program + 1;

        programs[i] = program;
    }

    return programs_size;
}

// fuuuu
int global_pid;
void simplesh_sigint_sigquit_handler(int signal)
{
    // terminate child process
    if (global_pid != 0 && kill(global_pid, SIGINT) < 0)
    {
        perror("kill with SIGINT error");
    }
    
    global_pid = 0;
    // if needed terminate
    if (signal == SIGQUIT)
    {
        // TODO free memory
        exit(EXIT_SUCCESS);
    }
}

int main()
{
    global_pid = 0;

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = simplesh_sigint_sigquit_handler;
    sigset_t   set; 
    sigemptyset(&set);                                                             
    sigaddset(&set, SIGINT); 
    sigaddset(&set, SIGQUIT);
    act.sa_mask = set;
    if (sigaction(SIGINT, &act, 0) < 0)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    if (sigaction(SIGQUIT, &act, 0) < 0)
    {
        perror("sigaction");
        return EXIT_FAILURE;
    }
    
    struct buf_t* buf = buf_new(BUF_SIZE);
    if (buf == NULL)
    {
        perror("malloc");
        return EXIT_FAILURE;
    }

    char line[BUF_SIZE];
    ssize_t len = 0;

    if (write(STDOUT_FILENO, "$", 1) < 0)
    {
        perror("write");
        return EXIT_FAILURE;
    }
    while ((len = buf_getline(STDIN_FILENO, buf, line)) > 0)
    {
        execargs_t** programs;
        ssize_t programs_size = get_execargs(line, len, programs);
        if (programs_size == -1)
        {
            perror("get_execargs");
            return EXIT_FAILURE;
        }

        global_pid = fork();
        if (pid == 0)
        {
            runpiped(programs, programs_size);
        }
        else if (pid > 0)
        {
            int status;
            if (wait(&status) < 0)
            {
                perror("runpiped exited with error");
            }

            if (write(STDOUT_FILENO, "\n$", 2) < 0)
            {
                perror("write");
                return EXIT_FAILURE;
            }
        }
        else
        {
            perror("fork");
            return EXIT_FAILURE;
        }
    }
    
    return EXIT_SUCCESS;
}


















