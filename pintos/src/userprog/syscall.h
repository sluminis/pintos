#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include "threads/thread.h"

void syscall_init(void);

/* System calls */
void k_exit (int status);

bool k_create (const char* file, unsigned initial_size);
bool k_remove (const char* file);
int k_open (const char* file);
int k_filesize (int fd);
int k_read (int fd, void* buffer, unsigned length);
int k_write (int fd, const void* buffer, unsigned length);
void k_seek (int fd, unsigned position);
unsigned k_tell (int fd);
void k_close (int fd);

#endif /* userprog/syscall.h */
