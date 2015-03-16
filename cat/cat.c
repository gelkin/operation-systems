#include <stdio.h>
#include <stdlib.h>
#include "helpers.c"

#define BUF_SIZE 1024

int main()
{
    char buf[BUF_SIZE];

    int read_bytes = 0;
    while ((read_bytes = read_(STDIN_FILENO, buf, BUF_SIZE)) > 0)
    {
        int on_write = write_(STDOUT_FILENO, buf, read_bytes);
        if (on_write < 0)
        {
            perror("Couldn't write");
            return EXIT_FAILURE;
        }
    }
    if (read_bytes < 0)
    {
        perror("Couldn't read");
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}
