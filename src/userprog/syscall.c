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
#include "threads/vaddr.h"
#include "pagedir.h"
#include "lib/user/syscall.h"
#include "filesys/filesys.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);
static struct lock file_lock;

static int
get_int(void *esp)
{
  int x;
  memcpy(&x, esp, 4);
  return x;
}

static void*
get_ptr(void *esp)
{
  int ptr;
  memcpy(&ptr, esp, 4);
  return (void *)ptr;
}

static void
validate_ptr(const void *ptr)
{
  if (ptr == NULL || ptr >= PHYS_BASE || pagedir_get_page(thread_current()->pagedir, ptr) == NULL)
  {
    // exit(-1);
  }
}

static void
halt_wrapper()
{
	shutdown_power_off();
}

static void
exit_wrapper(struct intr_frame *f)
{
  int status = get_int(f->esp + 4);

  struct thread *cur = thread_current ();
  
  struct list_elem *e = list_head (&cur->open_files);
  while ((e = list_next (e)) != list_end (&cur->open_files))
  {
    struct open_file *file = list_entry(e, struct open_file, elem);
    file_close(file->ptr);
  }

  file_allow_write(thread_current()->executable_file);

  e = list_head(&thread_current()->parent_thread->child_processes);
  struct child_process *child;
  while ((e = list_next (e)) != list_end (&cur->open_files))
  {
    child = list_entry(e, struct child_process, elem);
    if(child->pid == thread_current()->tid)
      break;
  }
  free(child);

  if (cur->parent_thread->waiting_on == cur->tid)
  {
    cur->parent_thread->status = status;
    cur->parent_thread->waiting_on = -1;
    sema_up(&cur->parent_thread->wait_child);
    thread_current()->parent_thread->child_status = 0;
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

static void
exec_wrapper(struct intr_frame *f)
{
  char *file_name = get_ptr(f->esp + 4);
  validate_ptr(file_name);
  f->eax = (uint32_t)process_execute(file_name);
}

static void
wait_wrapper(struct intr_frame *f)
{
  int tid = get_int(f->esp + 4);
  f->eax = process_wait(tid);
}

static void
create_wrapper(struct intr_frame *f)
{
  char *file_name = get_ptr(f->esp + 4);
  unsigned initial_size = get_int(f->esp + 8);
  validate_ptr(file_name);
  lock_acquire(&file_lock);
  f->eax = filesys_create(file_name, initial_size);
  lock_release(&file_lock);
}

static void
remove_wrapper(struct intr_frame *f)
{
  char *file_name = get_ptr(f->esp + 4);
  validate_ptr(file_name);
  lock_acquire(&file_lock);
  f->eax = filesys_remove(file_name);
  lock_release(&file_lock);
}

static void
open_wrapper(struct intr_frame *f)
{
  struct open_file* file = malloc(sizeof(struct open_file));
  file->fd = ++thread_current()->fd_last;

  char *file_name = get_ptr(f->esp + 4);
  validate_ptr(file_name);
  lock_acquire(&file_lock);
  file->ptr = filesys_open(file_name);
  lock_release(&file_lock);

  file_deny_write(file->ptr);
  list_push_back(&thread_current()->open_files, &file->elem);
}

static void
filesize_wrapper(struct intr_frame *f)
{
  int fd = get_int(f->esp + 4);
  
  struct list_elem *e = list_head(&thread_current()->open_files);
  struct open_file *file;
  while ((e = list_next(e) != list_end(&thread_current()->open_files)))
  {
    file = list_entry(e, struct open_file, elem);
    if (file->fd == fd)
      break;
  }

  f->eax = file_length(file->ptr);
}

static void
read_wrapper(struct intr_frame *f)
{
  int fd = get_int(f->esp + 4);
  void* buffer = get_ptr(f->esp + 8);
  unsigned size = get_int(f->esp + 12);

  if (fd == 0)
  {
    while (size--)
      *(uint8_t*)buffer++ = input_getc();

    f->eax = size;
  }
  else if (fd == 1)
  {
    // negative area
  }
  else
  {
    struct list_elem *e = list_head(&thread_current()->open_files);
    struct open_file *file;
    while ((e = list_next(e) != list_end(&thread_current()->open_files)))
    {
      file = list_entry(e, struct open_file, elem);
      if (file->fd == fd)
        break;
    }
    f->eax = file_read(file->ptr, buffer, size);
  }
}

static void
write_wrapper(struct intr_frame *f)
{
  int fd = get_int(f->esp + 4);
  char* buffer = get_ptr(f->esp + 8);
  unsigned size = get_int(f->esp + 12);
  validate_ptr(buffer);

  if (fd == 1)
  {
    lock_acquire(&file_lock);
    putbuf(buffer, size);
    lock_release(&file_lock);

    f->eax = size;
  }
  else if (fd == 0)
  {
    // negative area
  }
  else
  {
    struct list_elem *e = list_head(&thread_current()->open_files);
    struct open_file *file;
    while ((e = list_next(e) != list_end(&thread_current()->open_files)))
    {
      file = list_entry(e, struct open_file, elem);
      if (file->fd == fd)
        break;
    }
    f->eax = file_write(file->ptr, buffer, size);
  }
}

static void
seek_wrapper(struct intr_frame *f)
{
  int fd = get_int(f->esp + 4);
  int pos = get_int(f->esp + 8);

  struct list_elem *e = list_head(&thread_current()->open_files);
  struct open_file *file;
  while ((e = list_next(e) != list_end(&thread_current()->open_files)))
  {
    file = list_entry(e, struct open_file, elem);
    if (file->fd == fd)
      break;
  }

  file_seek(file->ptr, pos);
}

static void
tell_wrapper(struct intr_frame *f)
{
  int fd = get_int(f->esp + 4);

  struct list_elem *e = list_head(&thread_current()->open_files);
  struct open_file *file;
  while ((e = list_next(e) != list_end(&thread_current()->open_files)))
  {
    file = list_entry(e, struct open_file, elem);
    if (file->fd == fd)
      break;
  }

  f->eax = file_tell(file->ptr);
}

static void
close_wrapper(struct intr_frame *f)
{
  int fd = get_int(f->esp + 4);

  struct list_elem *e = list_head(&thread_current()->open_files);
  struct open_file *file;
  while ((e = list_next(e) != list_end(&thread_current()->open_files)))
  {
    file = list_entry(e, struct open_file, elem);
    if (file->fd == fd)
      break;
  }

  file_close(file->ptr);

  list_remove(e);
  free(file);
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
    case SYS_HALT:
      halt_wrapper(f);
      break;

    case SYS_EXIT:
      exit_wrapper(f);
      break;

    case SYS_EXEC:
      exec_wrapper(f);
      break;

    case SYS_WAIT:
      wait_wrapper(f);
      break;

    case SYS_CREATE:
      create_wrapper(f);
      break;
    
    case SYS_REMOVE:
      remove_wrapper(f);
      break;

    case SYS_OPEN:
      open_wrapper(f);
      break;

    case SYS_FILESIZE:
      filesize_wrapper(f);
      break;

    case SYS_READ:
      read_wrapper(f);
      break;

    case SYS_WRITE:
      write_wrapper(f);
      break;

    case SYS_SEEK:
      seek_wrapper(f);
      break;

    case SYS_TELL:
      tell_wrapper(f);
      break;

    case SYS_CLOSE:
      close_wrapper(f);
      break;
    
    default:
      break;
  }
  thread_exit();
}


