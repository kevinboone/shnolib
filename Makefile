TARGET  := shnolib 
# --gc-sections removes unusueds (sections) functions
# --ffunction-sections puts each function in its own section, so
#   the linker can easily remove unusued functions
CFLAGS  := -O3 -Wall -fno-builtin -ffunction-sections -fdata-sections 

all: $(TARGET) 

uname_m := $(shell uname -m)
ifeq ($(uname_m),x86_64)
  CRT := cnolib_amd64
else
ifeq ($(uname_m),armv7l)
  CRT := cnolib_arm
else
  $(error Unsupported architecture)
endif
endif

$(CRT).o: $(CRT).S
	as -o $(CRT).o -c $(CRT).S

cnolib.o: cnolib.c cnolib.h
	gcc $(CFLAGS) -o cnolib.o -c cnolib.c

main.o: main.c
	gcc $(CFLAGS) -o main.o -c main.c

$(TARGET): $(CRT).o cnolib.o shnolib.o
	ld --gc-sections -s -o $(TARGET) cnolib.o $(CRT).o shnolib.o

clean:
	rm -f *.o $(TARGET) foo*

