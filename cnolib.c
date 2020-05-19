/*===========================================================================

  shnolib -- a shell without a standard C library 

  C component of Kevin's tiny C library

  cnolib.c
  
  Copyright (c)2020 Kevin Boone. Distributed uner the terms of the 
    GNU PUblic Licence, v3.0

===========================================================================*/
#include "cnolib.h"

/*===========================================================================

  Structures

  Note that FILE is opaque in this implementation of buffered I/O. Since
    every function that works on FILE * is implemented in this one C source,
    there's no need to expose it to callers in a header.

===========================================================================*/
typedef enum __io_dir
  {
  _IODIR_IN = 0,
  _IODIR_OUT = 1
  } _io_dir;

typedef struct _FILE
  {
  int fd;
  int pos;
  _io_dir dir;
  BOOL error;
  BOOL eof;
  unsigned char buff[BUFSIZ];
  } FILE;


// Reference to the end of the uninitialized data segment, 
//  provided by the compiler. We need to define this as some type but,
//  in fact, we only refer to it by its pointer
extern char end;

// The traditional errno
int errno = 0;

// Pointer to the environment, derived in __main
char **envp;

// stdin, etc, FILE * initialized in __main()
FILE *stdin, *stdout, *stderr;

// We need to define a reference to the program's main(), so we can
//   call it from __main()
extern int main (int argc, char **argv);

/*===========================================================================

 __main 

 This function is called from __start in the assembly-code module. It
 just does some initialization before calling main.

===========================================================================*/
int __main (int argc, char **argv)
  {
  // Initialize the environment from the data _after_ argv[argc].
  //   This is data that was put on the stack by the kernel
  envp = &(argv[argc + 1]);

  // We would initialize the memory management system here, if it was
  //  sophisticated enough to need any initialization

  stdin = fdopen (STDIN_FILENO, "r"); 
  stdout = fdopen (STDOUT_FILENO, "w"); 
  stderr = fdopen (STDERR_FILENO, "w"); 

  return main (argc, argv);
  }

/*===========================================================================

  getenv 

===========================================================================*/
char *getenv (const char *name)
  {
  char *ret = NULL;
  char **ptr = envp;
  for ( ; (*ptr != NULL) && (ret == NULL); ptr++ )
    {
    const char *env = *ptr;
    char *eq = strchr (env, '='); 
    if (eq)
      {
      if (strncmp (name, env, eq - env) == 0)
        ret = eq + 1;
      }
    }
  return ret;  
  }

/*===========================================================================

  chdir 

===========================================================================*/
int chdir (const char *dir)
  {
  int r = syscall (SYS_CHDIR, dir);
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  }

/*===========================================================================

  execv

===========================================================================*/
extern int execv (const char *filename, char *const argv[])
  {
  return execve (filename, argv, envp);
  }

/*===========================================================================

  execve

===========================================================================*/
extern int execve(const char *filename, char *const argv[],
                  char *const envp[])
  {
  int r = syscall (SYS_EXECVE, filename, argv, envp);
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  }

/*===========================================================================

  execvp

===========================================================================*/
extern int execvp (const char *filename, char *const argv[])
  {
  // If the filename contains a separator, don't search $PATH
  if (strchr (filename, '/'))
    return execv (filename, argv);

  int lfilename = strlen (filename);
  const char *path = getenv ("PATH"); 
  if (path == NULL) path = "/bin:/usr/bin";
  char *pathv = strdup (path);
  char *tok = strtok (pathv, ":");
  while (tok)
    {
    // Allow space for separator and null
    char *try = malloc (strlen (tok) + lfilename + 2);
    strcpy (try, tok);
    strcat (try, "/");
    strcat (try, filename);
    if (access (try, X_OK) == 0)
      {
      return execv (try, argv);
      }
    free (try);
    tok = strtok (NULL, ":");
    };
  free (pathv);

  errno = ENOENT;
  return -1;
  }

/*===========================================================================

  fork 

===========================================================================*/
int fork (void)
  {
  int r = syscall (SYS_FORK);
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  }

/*===========================================================================

  exit 

===========================================================================*/
void exit (int status)
  {
  syscall (SYS_EXIT, status);
  // Ugh -- gcc recognizes "exit" as a "noreturn" function by default. So
  //   we have to do something that the compiler things is non-returning.
  // This code will ever be reached, because syscall 60 terminates the
  //   program. However, the compiler doesn't know that.
  for(;;);
  }

/*===========================================================================

  wait4 

===========================================================================*/
pid_t wait4 (pid_t pid, int *status, int options, struct rusage *rusage)
  {
  int r = syscall (SYS_WAIT4, pid, status, options, rusage);
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  } 

/*===========================================================================

  waitpid 

===========================================================================*/
pid_t waitpid (pid_t pid, int *wstatus, int options)
  {
  return wait4 (pid, wstatus, options, NULL);
  }

/*===========================================================================

  String handling functions

===========================================================================*/
/*===========================================================================

  strcat

===========================================================================*/
extern char *strcat (char *dest, const char *src)
  {
  strcpy (dest + strlen (dest), src);
  return dest;
  }

/*===========================================================================

  strcpy

===========================================================================*/
extern char *strcpy (char *dest, const char *src)
  {
  while (*src)
    {
    *dest = *src;
    dest++; src++;
    } 
  *dest = 0;
  return dest;
  }

/*===========================================================================

  strdup 

===========================================================================*/
char *strdup (const char *s)
  {
  char *ret = malloc (strlen (s) + 1);
  strcpy (ret, s);
  return ret;
  }

/*===========================================================================

  strlen

===========================================================================*/
size_t strlen (const char *str)
  {
  for (size_t len = 0; ; ++len) 
    if (str[len] == 0) return len;
  return 0;
  }

/*===========================================================================

  strchr

===========================================================================*/
char *strchr (const char *s, int c)
  {
  for (; *s != '\0' && *s != c; ++s)
    ;
  return *s == c ? (char *) s : NULL;
  }

/*===========================================================================

  strnchr

===========================================================================*/
char *strnchr (const char *s, int c, size_t n)
  {
  int i = 0;
  for (; *s != '\0' && *s != c && ++i < n; ++s)
    ;
  return *s == c ? (char *) s : NULL;
  }

/*===========================================================================

  strcmp

===========================================================================*/
int strcmp (const char *s1, const char *s2)
  {
  while (*s1 && (*s1 == *s2))
    {
    ++s1;
    ++s2;
    }
  return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
  }

/*===========================================================================

  strncmp

===========================================================================*/
int strncmp (const char *s1, const char *s2, size_t n)
  {
  while (n && *s1 && (*s1 == *s2))
    {
    ++s1;
    ++s2;
    --n;
    }
  if (n == 0)
    {
    return 0;
    }
  else
    {
    return ( *(unsigned char *)s1 - *(unsigned char *)s2 );
    }
  }

/*===========================================================================

  strpbrk

===========================================================================*/
char *strpbrk (const char *s, const char *accept)
  {
  while (*s != '\0')
    {
    const char *a = accept;
    while (*a != '\0')
      if (*a++ == *s)
        return (char *) s;
     ++s;
    }
  return NULL;
  }

/*===========================================================================

  strspn

===========================================================================*/
size_t strspn (const char *s, const char *accept)
  {
  const char *p;
  const char *a;
  size_t count = 0;

  for (p = s; *p != '\0'; ++p)
    {
    for (a = accept; *a != '\0'; ++a)
    if (*p == *a)
      break;
    if (*a == '\0')
      return count;
    else
      ++count;
    }

  return count;
  }

/*===========================================================================

  strtok

===========================================================================*/
static char *olds;
char *strtok (char *s, const char *delim)
  {
  char *token;

  if (s == NULL)
    s = olds;

  s += strspn (s, delim);
  if (*s == '\0')
    {
    olds = s;
    return NULL;
    }

  token = s;

  s = strpbrk (token, delim);
  if (s == NULL)
    {
    olds = rawmemchr (token, '\0');
    }
  else
    {
    *s = '\0';
    olds = s + 1;
    }
  return token;
  }

/*===========================================================================

  reverse
  Reverse the bytes in a char* of specified length. The
  data need not be null-terminated

===========================================================================*/
void reverse (char str[], int length) 
  { 
  int start = 0; 
  int end = length -1; 
  while (start < end) 
    { 
    SWAP (*(str+start), *(str+end)); 
    start++; 
    end--; 
    } 
  } 

/*===========================================================================

  ltoa 

===========================================================================*/
char *ltoa (long n, char *str, int base)
  {
  long i = 0; 
  BOOL isNegative = FALSE; 
  
  if (n == 0) 
    { 
    str[i++] = '0'; 
    str[i] = '\0'; 
    return str; 
    } 
  
  if (n < 0 && base == 10) 
    { 
    isNegative = TRUE; 
    n = -n; 
    } 
  
  while (n != 0) 
    { 
    int rem = n % base; 
    str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0'; 
    n = n/base; 
    } 
  
  if (isNegative) 
    str[i++] = '-'; 
  
  str[i] = '\0'; // Append string terminator 
  
  reverse (str, i); 
  
  return str; 
  } 


/*===========================================================================

  itoa 

  We can just call ltoa, because there's nothing in that function which
  is dependent on the data size.

===========================================================================*/
char *itoa (int n, char *str, int base)
  {
  return ltoa (n, str, base);
  } 


/*===========================================================================

  Memory management 

  Warning: very, very crude and inefficient. This is really only a
  toy implementation at present

===========================================================================*/
typedef struct free_block 
  {
  size_t size;
  struct free_block* next;
  } free_block;

static free_block free_block_list_head = { 0, 0 };
static const size_t align_to = 16;

/*===========================================================================

  brk 

===========================================================================*/
int brk (void *addr)
  {
  int x = sys_brk ((unsigned long)addr);
  if ((unsigned long) x > (unsigned long)addr) return 0;
  return -1;
  }

/*===========================================================================

  sbrk 

===========================================================================*/
void *sbrk (intptr_t increment)
  {
  void *old = (void *) ((uintptr_t)sys_brk (0));
  void *new = (void *) (uintptr_t)sys_brk ((uintptr_t)old + increment);
  return (((uintptr_t)new) == (((uintptr_t)old) + increment)) ? old :
         (void *)-1;
  }

/*===========================================================================

  malloc 

===========================================================================*/
void *malloc (size_t size)
  {
  // Align size of 16-byte boundary
  size = (size + sizeof(size_t) + (align_to - 1)) & ~ (align_to - 1);
  free_block* block = free_block_list_head.next;
  free_block** head = &(free_block_list_head.next);
  while (block != 0) 
    {
    if (block->size >= size) 
      {
      *head = block->next;
      return ((char*)block) + sizeof(size_t);
      }
    head = &(block->next);
    block = block->next;
    }

  block = (free_block*)sbrk(size);
  block->size = size;

  return ((char*)block) + sizeof(size_t);
  }

/*===========================================================================

  free 

===========================================================================*/
void free (void* ptr) 
  {
  free_block* block = (free_block*)(((char*)ptr) - sizeof(size_t));
  block->next = free_block_list_head.next;
  free_block_list_head.next = block;
  }

/*===========================================================================

  memchr

===========================================================================*/
void *memchr(const void *s, int c, size_t n)
  {
  // Just delegate to strnchr()
  return strnchr (s, c, n);
  }

/*===========================================================================

  memcpy

===========================================================================*/
void *memcpy (void *dest, const void *src, size_t n)
  {
  for (register int i = 0; i < n; i++)
    ((char *)(dest))[i] = ((char *)(src))[i];
  return dest;
  }

/*===========================================================================

  memmove

===========================================================================*/
void *memmove (void *dest, const void *src, size_t n)
  {
  char *d = (char*)dest;
  char *s = (char*)src;
  if (s < d) 
    {
    s += n;
    d += n;
    while (n--)
      *--d = *--s;
    } 
  else 
    {
    while (n--)
      *d++ = *s++;
    }

  return dest;
  }

/*===========================================================================

  memset

===========================================================================*/
extern void *memset (void *s, int c, size_t n)
  {
  for (register int i = 0; i < n; i++)
    ((char *)(s))[i] = c; 
  return s;
  }

/*===========================================================================

  rawmemchr

===========================================================================*/
void *rawmemchr(const void *s, int c)
  {
  // Just delegate to strchr()
  return strchr (s, c);
  }

/*===========================================================================

  _cnolib_dump_mem_blocks

===========================================================================*/
extern void _cnolib_dump_mem_blocks (void)
  {
/*
  free_block* block = free_block_list_head.next;
  free_block** head = &(free_block_list_head.next);
  while (block != 0) 
    {
    head = &(block->next);
    block = block->next;
    }
*/
  }

/*===========================================================================

  basic I/O 

===========================================================================*/
/*===========================================================================

  putchar

===========================================================================*/
int putchar (int c)
  {
  write (1, &c, 1);
  return c;
  }

/*===========================================================================

  puts 

===========================================================================*/
int puts (const char *s)
  {
  int r = write (1, s, strlen (s));
  r += write (1, "\n", 1);
  return r;
  }

/*===========================================================================

  close 

===========================================================================*/
int close (int fd)
  {
  int r = syscall (SYS_CLOSE, fd);
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  }

/*===========================================================================

  open 

===========================================================================*/
int open (const char *s, int flags)
  {
  int r = syscall (SYS_OPEN, s, flags, 0777);
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  }

/*===========================================================================

  write 

===========================================================================*/
int write (int fd, const void *buff, int l)
  {
  int r = syscall (SYS_WRITE, fd, buff, l); 
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  }


/*===========================================================================

 read 

===========================================================================*/
int read (int fd, const void *buff, int l)
  {
  int r = syscall (SYS_READ, fd, buff, l); 
  if (r < 0) 
    {
    errno = -r;
    return -1;
    }
  else
    {
    errno = 0;
    return r;
    }
  }


/*===========================================================================

 buffered IO 

===========================================================================*/
/*===========================================================================


 fclose

===========================================================================*/
int fclose (FILE *f)
  {
  fflush (f); 
  free (f);
  return 0;
  }

/*===========================================================================


 fdopen

===========================================================================*/
FILE *fdopen (int fd, const char *mode)
  {
  FILE *ret = NULL;
  ret = malloc (sizeof (FILE));
  if (mode[0] == 'a' || mode[0] == 'w')
    ret->dir = _IODIR_OUT;
  else
    ret->dir = _IODIR_IN;
  memset (ret->buff, 0, BUFSIZ);
  ret->pos = 0;
  ret->fd = fd;
  ret->eof = FALSE;
  ret->error = FALSE;
  return ret;
  }

/*===========================================================================

 fflush

===========================================================================*/
extern int fflush (FILE *f)
  {
  int ret = 0;

  switch (f->dir)
    {
    case _IODIR_IN:
      f->pos = 0; // Simply ignore any accumulated data
      break;
    case _IODIR_OUT:
      // Write the accumulated data to file
      if (write (f->fd, f->buff, f->pos) != 0)
        {
        ret = -1; // errno set by write()
        }
      break;
    }

  f->pos = 0;
  return ret;
  }

/*===========================================================================

 fgetc

===========================================================================*/
int fgetc (FILE *f)
  {
  int ret = 0;
  int done = FALSE;
  int error = FALSE;
  int eof = FALSE;
  do
    {
    if (f->pos > 0)
      {
      ret = (int)f->buff[0]; 
      memmove (f->buff, f->buff + 1, BUFSIZ - 1);
      f->pos --;
      done = TRUE;
      }
    else
      {
      int tofill = BUFSIZ - f->pos;
      int r = read (f->fd, f->buff +  f->pos, tofill); 
      if (r < 0)
        {
        error = TRUE;
        }
      else if (r == 0)
        {
        ret = EOF;
        eof = TRUE;
        }
      else if (r < tofill)
        {
        // We didn't fill the buffer, but maybe we have a character 
        f->pos += r;
        }
      else
        {
        // We filled the buffer -- carry on 
        }
      }
    } while (!done & !error & !eof);
  return ret;
  }

/*===========================================================================

 ferror

===========================================================================*/
int ferror (FILE *f)
  {
  return f->error;
  }

/*===========================================================================

 feof

===========================================================================*/
int feof (FILE *f)
  {
  return f->eof;
  }

/*===========================================================================

 fgets

===========================================================================*/
char *fgets (char *s, int size, FILE *f)
  {
  char *ret = NULL;
  int done = FALSE;
  int error = FALSE;
  int eof = FALSE;
  do
    {
    char *eol = strnchr ((char *)f->buff, '\n', f->pos); 
    if (eol)
      {
      int eoloff = eol - (char *)f->buff + 1;
      int tocopy = eoloff < size - 2 ? eoloff : size - 2; 
      memcpy (s, (char *)f->buff, tocopy); 
      memmove (f->buff, f->buff + tocopy, BUFSIZ - tocopy);
      f->pos -= tocopy;
      s[tocopy] = 0;
      ret = s;
      done = TRUE;
      }
    else if (eof)
      {
      if (f->pos > 0)
        {
        int eoloff = f->pos; 
        int tocopy = eoloff < size - 1 ? eoloff : size - 1; 
        memcpy (s, (char *)f->buff, tocopy); 
        memmove (f->buff, f->buff + tocopy, tocopy);
        f->pos -= tocopy;
        s[tocopy] = 0;
        ret = s;
        }
      else
        {
        ret = NULL;
        f->error = TRUE;
        f->eof = TRUE;
        }
      done = TRUE;
      }
    else if (!eof)
      {
      int tofill = BUFSIZ - f->pos;
      int r = read (f->fd, f->buff + f->pos, tofill); 
      if (r < 0)
        {
        error = TRUE;
        f->error = TRUE;
        ret = NULL;
        }
      else if (r < tofill)
        {
        // We didn't fill the buffer, but maybe we have a line
        f->pos += r;
        eof = TRUE;
        }
      else
        {
        // We filled the buffer -- carry on and see if we have a line
        f->pos += r;
        }
      }
    else
      {
      done = TRUE;
      }
    } while (!done & !error);
  return ret;
  }

/*===========================================================================

 fopen 

===========================================================================*/
extern FILE *fopen (const char *filename, const char *mode)
  {
  FILE *ret = NULL;

  int flags = 0;
  switch (mode[0])
    {
    case 'r': 
      flags = O_RDONLY;
      break;
    case 'a': 
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    case 'w': 
      flags = O_WRONLY | O_CREAT;
      break;
    } 


  int fd = open (filename, flags);

  if (fd >= 0)
    {
    ret = fdopen (fd, mode);
    }
  return ret;
  }


/*===========================================================================

 fread

===========================================================================*/
size_t fread (void *_ptr, size_t size, size_t n, FILE *f)
  {
  // This is a very simply implementation -- it's no implementation at all
  int ret = 0;
  size_t total = size * n;

  int r = read (f->fd, _ptr, total);
  if (r < 0)
    {
    f->error = TRUE;
    ret = 0;
    }
  else if (r == 0)
    {
    f->eof = TRUE;
    ret = 0;
    }
  else
    {
    ret = r / size;
    }

  return ret;
  }

/*===========================================================================

 _fwrite 

===========================================================================*/
static int _fwrite (const char *ptr, size_t size, FILE *f)
  {
  int ret = 0;
  for (int i = 0; (i < size) && (ret == 0); i++)
    {
    // TODO: split on \n with appropriate stream type
    f->buff[f->pos++] = ptr[i];
    if (f->pos == BUFSIZ)
      {
      ret = fflush (f); 
      f->pos = 0;
      }
    }
  return ret;
  }



/*===========================================================================

 fputs 

===========================================================================*/
int fputs (const char *s, FILE *f)
  {
  int l = strlen (s);
  return _fwrite (s, l, f);
  }

/*===========================================================================

 fwrite 

===========================================================================*/
size_t fwrite (const void *_ptr, size_t size, size_t n, FILE *f)
  {
  int i = 0;
  int ok = TRUE;
  const char *ptr = _ptr;
  for (i = 0; i < n && ok; i++)
    {
    if (_fwrite (ptr, size, f) != 0)
      ok = FALSE;
    ptr += size;
    // TODO set error
    }
  return i;
  }


/*===========================================================================

 File status 

===========================================================================*/
/*===========================================================================

 access 

===========================================================================*/
int access (const char *pathname, int mode)
  {
  return syscall (SYS_ACCESS, pathname, mode);
  }

/*===========================================================================

  error_handling 

===========================================================================*/
int sys_nerr = 34;
const char *const sys_errlist[] = 
  {
  "No error", 
  "Operation not permitted",
  "No such file or directory",
  "No such process",
  "Interrupted system call",
  "I/O error",
  "No such device or address",
  "Argument list too long",
  "Exec format error",
  "Bad file number",
  "No child processes",
  "Try again",
  "Out of memory",
  "Permission denied",
  "Bad address",
  "Block device required ",
  "Device or resource busy",
  "File exists",
  "Cross-device link",
  "No such device",
  "Not a directory",
  "Is a directory",
  "Invalid argument",
  "File table overflow",
  "Too many open files",
  "Not a typewriter",
  "Text file busy",
  "File too large",
  "No space left on device",
  "Illegal seek",
  "Read-only file system",
  "Too many links",
  "Broken pipe",
  "Math argument out of domain of func",
  "Math result not representable"
  };

/*===========================================================================

  perror 

===========================================================================*/
void perror (const char *message)
  {
  int _errno = errno;
  if (message && message[0])
    {
    fputs (message, stderr);
    fputs (": ", stderr);
    }
  fputs (strerror (_errno), stderr);
  fputs ("\n", stderr);
  fflush (stderr);
  }

/*===========================================================================

  strerror 

===========================================================================*/
static char _errbuf [128];

char *strerror (int errnum)
  {
  if (errnum < sys_nerr)
    {
    strcpy (_errbuf, sys_errlist[errnum]);
    }
  else
    {
    strcpy (_errbuf, "Error ");
    itoa (errnum, _errbuf + strlen (_errbuf), 10);
    }

  return _errbuf;
  }





