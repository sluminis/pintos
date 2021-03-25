#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

void syscall_init(void);

/* System calls */
void k_halt (void);
void k_exit (int status);
tid_t k_exec (const char* file);
int k_wait (tid_t pid);

bool k_create (const char* file, unsigned initial_size);
bool k_remove (const char* file);
int k_open (const char* file);
int k_filesize (int fd);
int k_read (int fd, void* buffer, unsigned length);
int k_write (int fd, const void* buffer, unsigned length);
void k_seek (int fd, unsigned position);
unsigned k_tell (int fd);
void k_close (int fd);
int k_practice (int i);

#endif /* userprog/syscall.h */
