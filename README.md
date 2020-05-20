# shnolib

A demonstration how to create a _very_ simple Linux shell, without a standard
C library. The file cnolib.c provides (rather crude) implementations of
the standard library functions that the program needs. cnolib\_XXX.S are 
the assembly-code modules that provides the entry point, \_start, which
calls main(). These modules also provide a generic way for the C code
to invoke kernel syscalls. 

At this time, there are assembly code modules for ARMv7 (e.g., Raspberrry Pi)
and AMD64 (64-bit Intel and similar).

For more details, please see

http://kevinboone.me/shnolib.html

