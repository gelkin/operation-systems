#include <unistd.h>
#include "helpers.h"

ssize_t read_(int fd, void *buf, size_t count) 
{
    ssize_t read_bytes = 0;
    ssize_t offset = 0;
    while ((read_bytes = read(fd, buf + offset, count - offset)) > 0)
    {
       offset += read_bytes;
    }
    
    if (read_bytes == 0)
    {
        return offset;
    }
    else
    {
        return read_bytes;
    }
}

ssize_t write_(int fd, const void *buf, size_t count) 
{
    ssize_t written_bytes = 0;
    ssize_t offset = 0;
    while ((written_bytes = write(fd, buf + offset, count - offset)) > 0)
    {
       offset += written_bytes;
    }
    
    if (written_bytes == 0)
    {
        return offset;
    }
    else
    {
        return written_bytes;
    }
}
