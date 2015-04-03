
#include "bufio.h"

struct buf_t
{
    size_t size;
    size_t capacity;
    char* buffer;
};

struct buf_t *buf_new(size_t capacity)
{
    struct buf_t* res = (struct buf_t*) malloc(sizeof(struct buf_t));
    if (res == NULL)
    {
        return NULL;
    }
    
    char* buffer = (char*) malloc(capacity * sizeof(char));
    if (buffer == NULL)
    {
        return NULL;
    }

    res->capacity = capacity;
    res->size = 0;
    res->buffer = buffer;

    return res;
}

void buf_free(struct buf_t * buffer)
{
    if (buffer->buffer != NULL)
    {
        free(buffer->buffer);
    }
    
    free(buffer);
}

size_t buf_capacity(struct buf_t * buffer)
{
#ifdef DEBUG
    if (buffer == NULL) 
    {
        abort();
    }
#endif

    return buffer->capacity;
}

size_t buf_size(struct buf_t * buffer)
{
#ifdef DEBUG
    if (buffer == NULL) 
    {
        abort();
    }
#endif

    return buffer->size;
}

ssize_t buf_fill(fd_t fd,struct buf_t *buf, size_t required)
{
#ifdef DEBUG
    if (buf == NULL) 
    {
        abort();
    }
#endif

    ssize_t read_bytes = 0;
    while ((buf->size < required) &&
           (read_bytes = read(fd, buf->buffer + buf->size, buf->capacity - buf->size)) > 0)
    {
       buf->size += read_bytes; 
    }

    if (read_bytes >= 0)
    {
        return buf->size;
    }
    else
    {
        // Only read-error may appear, so errno is already set as read-error.
        return -1;
    }
}

ssize_t buf_flush(fd_t fd, struct buf_t *buf, size_t required)
{
#ifdef DEBUG
    if (buf == NULL) 
    {
        abort();
    }
#endif

    ssize_t written_bytes = 0;
    size_t offset = 0;
    while ((offset < required) &&
           (written_bytes = write(fd, buf->buffer + offset, buf->size - offset)))
    {
        offset += written_bytes;
    }
    
    if (written_bytes >= 0)
    {
        memmove(buf->buffer, buf->buffer + offset, buf->size - offset);
        buf->size = buf->size - offset;
        return offset; // (old_size - new_size)
    }
    else
    {
        return -1;
    }
}
