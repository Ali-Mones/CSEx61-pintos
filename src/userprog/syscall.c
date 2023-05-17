#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}
// TODO functions of get int and get pointers <3

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  switch (f->vec_no)
  {
    case SYS_EXEC:
      process_execute();
      break;
    
    default:
      break;
  }
  thread_exit ();
}


// static void
// wait_wrapper()
// {

// }