#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include "lib/kernel/stdio.h"
#include "lib/stdio.h"

static void syscall_handler (struct intr_frame *);
static struct lock file_lock;

static int
get_int(void *esp)
{
  int x;
  memcpy(&x, esp, 4);
  return x;
}

// static char*
// get_char_ptr(void **esp)
// {
//   int size = strlen((char*)*esp);
//   char* string = malloc(size);
//   memcpy(string, *esp, size);
//   *esp += size;
//   return string;
// }

static void*
get_ptr(void *esp)
{
  int ptr;
  memcpy(&ptr, esp, 4);
  return ptr;
}

static void
validate_ptr(const void *ptr)
{
}

static void
exec_wrapper(struct intr_frame *f)
{
  char *file_name = get_ptr(f->esp);
  f->eax = (uint32_t)process_execute(file_name);
}

static void
wait_wrapper(struct intr_frame *f)
{
  int tid = get_int(f->esp);
  process_wait(tid);
}

static uint32_t
write_wrapper(struct intr_frame *f)
{
  int fd = get_int(f->esp + 4);
  char* buffer = get_ptr(f->esp + 8);
  unsigned size = get_int(f->esp + 12);
  validate_ptr(buffer);
  if (fd == 1)
  {

    putchar('x');
    lock_acquire(&file_lock);
    putbuf(buffer, size);
    lock_release(&file_lock);
  }

  return strlen(buffer);
}

static void
exit_wrapper(struct intr_frame *f)
{
  int status = get_int(f->esp);

  struct thread *cur = thread_current ();
  
  struct list_elem *e = list_head (&cur->open_files);
  while ((e = list_next (e)) != list_end (&cur->open_files))
  {
    struct open_file *file = list_entry(e, struct open_file, elem);
    file_close(file->ptr);
  }

  if (cur->parent_thread->waiting_on == cur->tid)
  {
    cur->parent_thread->status = status;
    cur->parent_thread->waiting_on = -1;
    sema_up(&cur->parent_thread->wait_child);
  }
  else
  {
    struct list_elem *e = list_head (&cur->parent_thread->child_processes);
    while ((e = list_next (e)) != list_end (&cur->parent_thread->child_processes))
    {
      struct child_process *child = list_entry(e, struct child_process, elem);
      if (child->pid == cur->tid)
        break;
    }
    list_remove(e);
  }

  e = list_head (&cur->child_processes);
  while ((e = list_next (e)) != list_end (&cur->child_processes))
    sema_up(&cur->child_parent_sync);

  thread_exit();
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&file_lock);
}

void
syscall_handler (struct intr_frame *f)
{
  int call_type = get_int(f->esp);
  switch (call_type)
  {
    case SYS_EXEC:
      exec_wrapper(f);
      break;

    case SYS_WAIT:
      wait_wrapper(f);
      break;

    case SYS_WRITE:
      f->eax = write_wrapper(f);
      break;

    case SYS_EXIT:
      exit_wrapper(f);
      break;
    
    default:
      break;
  }
  thread_exit ();
}


