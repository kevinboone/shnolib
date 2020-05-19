# shnolib

A demonstration how to create a _very_ simple Linux shell, without a standard
C library. The file cnolib.c provides (rather crude) implementations of
the standard library functions that the program needs. cnolib\_amd64.S is
the assembly-code module that provides the entry point, \_start, which
calls main(). This module also provides a generic way for the C code
to invoke kernel syscalls. 
The assembly-code module is specific to the AMD64 (x86-64) architecture. 
So far as I know, none of the C code is architecture-dependent.

For more details, please see

http://kevinboone.me/shnolib.html

