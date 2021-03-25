#include "userprog/syscall.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

static void syscall_handler(struct intr_frame*);

void syscall_init(void) { intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall"); }

/* Check variables */
static bool check_string(const char* string);
static bool check_readonly_buffer(const void* buffer, unsigned length);
static bool check_writable_buffer(void* buffer, unsigned length);
static bool check_uint32(uint32_t* p);
static bool check_args(uint32_t* p, unsigned n);

static void syscall_handler(struct intr_frame* f UNUSED) {
  uint32_t* args = ((uint32_t*)f->esp);
  if (!check_uint32(args)) k_exit(-1);

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
    if (!check_args(args, 2)) k_exit(-1);
    f->eax = args[1];
    k_exit(args[1]);
    break;

  case SYS_EXEC:
    if (!check_args(args, 2)) k_exit(-1);
    file = (const char*) args[1];
    if (!check_string(file)) k_exit(-1);
    f->eax = k_exec(file);
    break;

  case SYS_WAIT:
    if (!check_args(args, 2)) k_exit(-1);
    f->eax = k_wait(args[1]);
    break;

  case SYS_WRITE:
    if (!check_args(args, 4)) k_exit(-1);
    const_buffer = (const void*) args[2];
    size = args[3];
    if (!check_readonly_buffer(const_buffer, size)) k_exit(-1);
    f->eax = k_write(args[1], const_buffer, size);
    break;

  case SYS_PRACTICE:
    if (!check_args(args, 2)) k_exit(-1);
    f->eax = args[1] + 1;
    break;
  
  default:
    printf("%s: can't handle such syscall!\n", thread_current()->name);
    break;
  }
}

void k_exit (int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_exit();
}

void k_halt (void) {

}

tid_t k_exec (const char* file) {
  return 0;
}

int k_wait (tid_t pid) {
  return 0;
}

bool k_create (const char* file, unsigned initial_size) {
  return false;
}

bool k_remove (const char* file) {
  return false;
}

int k_open (const char* file) {
  return 0;
}

int k_filesize (int fd) {
  return 0;
}

int k_read (int fd, void* buffer, unsigned length) {
  return 0;
}

int k_write (int fd, const void* buffer, unsigned length) {
  if (fd == 1) {
    putbuf((const char*) buffer, length);
    return (int) length;
  } else if (fd <= 2) {
    return -1;
  }
  return 0;
}

void k_seek (int fd, unsigned position) {

}

unsigned k_tell (int fd) {
  return 0;
}

void k_close (int fd) {

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
  if (!check_addr(start, writable)) return false;
  void* next_page = pg_round_up(start);
  for (; (unsigned) (next_page - start) < length; next_page = pg_round_up(next_page + 1)) {
    if (!check_addr(next_page, writable)) return false;
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
