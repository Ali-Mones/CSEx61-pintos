#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct open_file
{
    struct file* ptr;
    int fd;
    struct list_elem elem;
};

struct child_process 
{
    tid_t pid;
    struct thread* t;
    struct list_elem elem;
};

tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
