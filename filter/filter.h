#ifndef FILTER_H
#define FILTER_H
char* get_line(char *buf, size_t n);
int process_buffer(char *buf, size_t size, int argc, char* argv[]);
int last_delim_pos(char *buf, size_t size);
void shift_left_args(int argc, char* argv[]);


#endif // FILTER_H
