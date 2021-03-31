#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "filesys/filesys.h"

static void syscall_handler(struct intr_frame*);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

/* Check variables */
static bool check_string(const char* string);
static bool check_readonly_buffer(const void* buffer, unsigned length);
static bool check_writable_buffer(void* buffer, unsigned length);
static bool check_uint32(uint32_t* p);
static bool check_args(uint32_t* p, unsigned n);

static void syscall_handler(struct intr_frame* f) {
  uint32_t* args = ((uint32_t*)f->esp);
  if (!check_uint32(args)) k_exit(EXIT_FAILURE);

  /*
   * The following print statement, if uncommented, will print out the syscall
   * number whenever a process enters a system call. You might find it useful
   * when debugging. It will cause tests to fail, however, so you should not
   * include it in your final submission.
   */

  // printf("System call number: %d\n", args[0]);
  unsigned size = 0;
  const char* file = NULL;
  const void* const_buffer = NULL;
  void* buffer = NULL;

  switch (args[0]) {
  case SYS_HALT:
    shutdown_power_off();
    break;
  
  case SYS_EXIT:
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    f->eax = args[1];
    k_exit(args[1]);
    break;

  case SYS_EXEC:
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    file = (const char*) args[1];
    if (!check_string(file)) k_exit(EXIT_FAILURE);
    f->eax = process_execute(file);
    break;

  case SYS_WAIT:
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    f->eax = process_wait(args[1]);
    break;

  case SYS_CREATE:   /* Create a file. */
    if (!check_args(args, 3)) k_exit(EXIT_FAILURE);
    file = (const char*) args[1];
    if (!check_string(file)) k_exit(EXIT_FAILURE);
    f->eax = k_create(file, args[2]);
    break;

  case SYS_REMOVE:   /* Delete a file. */
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    file = (const char*) args[1];
    if (!check_string(file)) k_exit(EXIT_FAILURE);
    f->eax = k_remove(file);
    break;

  case SYS_OPEN:     /* Open a file. */
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    file = (const char*) args[1];
    if (!check_string(file)) k_exit(EXIT_FAILURE);
    f->eax = k_open(file);
    break;

  case SYS_FILESIZE: /* Obtain a file's size. */
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    f->eax = k_filesize(args[1]);
    break;

  case SYS_READ:     /* Read from a file. */
    if (!check_args(args, 4)) k_exit(EXIT_FAILURE);
    buffer = (void*) args[2];
    size = args[3];
    if (!check_writable_buffer(buffer, size)) k_exit(EXIT_FAILURE);
    f->eax = k_read(args[1], buffer, size);
    break;

  case SYS_WRITE:    /* Write to a file. */
    if (!check_args(args, 4)) k_exit(EXIT_FAILURE);
    const_buffer = (const void*) args[2];
    size = args[3];
    if (!check_readonly_buffer(const_buffer, size)) k_exit(EXIT_FAILURE);
    f->eax = k_write(args[1], const_buffer, size);
    break;

  case SYS_SEEK:     /* Change position in a file. */
    if (!check_args(args, 3)) k_exit(EXIT_FAILURE);
    k_seek(args[1], args[2]);
    break;

  case SYS_TELL:     /* Report current position in a file. */
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    f->eax = k_tell(args[1]);
    break;

  case SYS_CLOSE:    /* Close a file. */
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    k_close(args[1]);
    break;

  case SYS_PRACTICE:
    if (!check_args(args, 2)) k_exit(EXIT_FAILURE);
    f->eax = args[1] + 1;
    break;
  
  default:
    printf("%s: can't handle such syscall!\n", thread_current()->name);
    break;
  }
}

void k_exit (int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit(status);
}

bool k_create (const char* file, unsigned initial_size) {
  bool res = false;

  file_operation_begin();
  res = filesys_create(file, initial_size);
  file_operation_end();

  return res;
}

bool k_remove (const char* file) {
  bool res = false;

  file_operation_begin();
  res = filesys_remove(file);
  file_operation_end();

  return res;
}

int k_open (const char* file) {
  int fd = BAD_FD;

  file_operation_begin();
  struct file* fs = filesys_open(file);
  if (fs != NULL) {
    fd = thread_allocate_fd();
    thread_current()->files[fd] = fs;
  }
  file_operation_end();

  return fd;
}

int k_filesize (int fd) {
  if (fd < 0 || fd > MAX_NUMBER_OF_FILES) k_exit(-1);
  struct thread* t = thread_current();
  int size = -1;

  file_operation_begin();
  if (t->files[fd] == NULL) {
    file_operation_end();
    k_exit(-1);
  }
  size = file_length(t->files[fd]);
  file_operation_end();
  
  return size;
}

int k_read (int fd, void* buffer, unsigned length) {
  if (fd < 0 || fd > MAX_NUMBER_OF_FILES) k_exit(-1);
  if (fd <= 2) {
    if (fd == 0) {
      for (unsigned i = 0; i < length; ++i) {
        ((uint8_t*) buffer)[i] = input_getc();
      }
      return length;
    }
    return -1;
  }

  struct thread* t = thread_current();
  int size = -1;

  file_operation_begin();
  if (t->files[fd] == NULL) {
    file_operation_end();
    k_exit(-1);
  }
  size = file_read(t->files[fd], buffer, length);
  file_operation_end();

  return size;
}

int k_write (int fd, const void* buffer, unsigned length) {
  if (fd < 0 || fd > MAX_NUMBER_OF_FILES) k_exit(-1);
  if (fd <= 2) {
    if (fd == 1) {
      putbuf((const char*) buffer, length);
      return (int) length;
    }
    return -1;
  }

  struct thread* t = thread_current();
  int size = -1;

  file_operation_begin();
  if (t->files[fd] == NULL) {
    file_operation_end();
    k_exit(-1);
  }
  size = file_write(t->files[fd], buffer, length);
  file_operation_end();

  return size;
}

void k_seek (int fd, unsigned position) {
  if (fd < 0 || fd > MAX_NUMBER_OF_FILES) k_exit(-1);
  struct thread* t = thread_current();
  
  file_operation_begin();
  if (t->files[fd] == NULL) {
    file_operation_end();
    k_exit(-1);
  }
  file_seek(t->files[fd], (off_t) position);
  file_operation_end();
}

unsigned k_tell (int fd) {
  if (fd < 0 || fd > MAX_NUMBER_OF_FILES) k_exit(-1);
  struct thread* t = thread_current();
  unsigned res = 0;

  file_operation_begin();
  if (t->files[fd] == NULL) {
    file_operation_end();
    k_exit(-1);
  }
  res = file_tell(t->files[fd]);
  file_operation_end();

  return res;
}

void k_close (int fd) {
  if (fd < 0 || fd > MAX_NUMBER_OF_FILES) k_exit(-1);
  struct thread* t = thread_current();

  file_operation_begin();
  if (t->files[fd] == NULL) {
    file_operation_end();
    k_exit(-1);
  }
  file_close(t->files[fd]);
  t->files[fd] = NULL;
  file_operation_end();
}

/* Check variables implementation. */
static bool check_range(void* buffer, unsigned length, bool writable);
static bool check_addr(void* addr, bool writable);
static int get_user (const uint8_t* uaddr);
static bool put_user (uint8_t* udst, uint8_t byte);

static bool check_string(const char* string) {
  for (; is_user_vaddr(string); ++string) {
    int res = get_user((const uint8_t*) string);
    if (res < 0) return false;
    if (res == 0) break;
  }
  return true;
}

static bool check_readonly_buffer(const void* buffer, unsigned length) {
  return check_range((void*) buffer, length, false);
}

static bool check_writable_buffer(void* buffer, unsigned length) {
  return check_range(buffer, length, true);
}

static bool check_uint32(uint32_t* p) {
  return check_range(p, sizeof(uint32_t), false);
}

static bool check_args(uint32_t* p, unsigned n) {
  return check_range(p, n*sizeof(uint32_t), false);
}

static bool check_range(void* start, unsigned length, bool writable) {
  void* page = start;
  for (; (unsigned) (page - start) < length; page = pg_round_up(page + 1)) {
    if (!check_addr(page, writable)) return false;
  }
  return true;
}

static bool check_addr(void* addr, bool writable) {
  if (!is_user_vaddr(addr)) return false;
  return writable ? put_user(addr, 0) : get_user(addr) != -1;
}

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */
static int get_user (const uint8_t* uaddr) {
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
        : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Writes BYTE to user address UDST.
   UDST must be below PHYS_BASE.
   Returns true if successful, false if a segfault occurred. */
static bool put_user (uint8_t* udst, uint8_t byte) {
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
        : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}
