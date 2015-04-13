#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "bufio.h"
#include "helpers.h"

#define BUF_SIZE 4096

int process_line(struct buf_t *buffer, char* line, size_t len, int argc, char* argv[])
{
    argv[argc - 1] = line;

    int res = spawn(argv[0], argv);
    if (res == 0)
    {
        // suppose that line has enough capacity
        line[len] = '\n';
        int on_write = buf_write(STDOUT_FILENO, buffer, line, len + 1);
        if (on_write < 0)
        {
            perror("Error while writing");
            return -1;
        }
    }
    else if (res < 0) 
    {
        perror("Error while calling 'spawn'");
        return -1;
    }

    return 1;
}

// shift args to the left EXEPT NULL-argument at the end
void shift_left_args(int argc, char* argv[])
{
    for (int i = 0; i < (argc - 1); ++i)
    {
        argv[i] = argv[i + 1];
    }
}

int main(int argc, char* argv[])
{
    shift_left_args(argc, argv);

    struct buf_t* buffer_in = buf_new(BUF_SIZE);
    struct buf_t* buffer_out = buf_new(BUF_SIZE);
    if (buffer_in == NULL || buffer_out == NULL)
    {
        perror("Malloc error");
        return EXIT_FAILURE;
    }
    
    char line[BUF_SIZE];
    ssize_t len = 0;
    
    while ((len = buf_getline(STDIN_FILENO, buffer_in, line)) > 0)
    {
        int on = process_line(buffer_out, line, len - 1, argc, argv);
        if (on < 0)
        {
            perror("Error while processing line");
            return EXIT_FAILURE;
        }
    }
    
    int on = buf_flush(STDOUT_FILENO, buffer_out, buf_size(buffer_out));
    buf_free(buffer_in);
    buf_free(buffer_out);

    if (on < 0)
    {
        perror("Error while flushing");
        return EXIT_FAILURE;
    }
    
    return (len == 0)? EXIT_SUCCESS: EXIT_FAILURE;
}
