TARGET  := shnolib 
# --gc-sections removes unusueds (sections) functions
# --ffunction-sections puts each function in its own section, so
#   the linker can easily remove unusued functions
CFLAGS  := -O3 -Wall -fno-builtin -ffunction-sections -fdata-sections 

all: $(TARGET) 

nolib_amd64.o: cnolib_amd64.S
	as -o cnolib_amd64.o -c cnolib_amd64.S

cnolib.o: cnolib.c
	gcc $(CFLAGS) -o cnolib.o -c cnolib.c

main.o: main.c
	gcc $(CFLAGS) -o main.o -c main.c

$(TARGET): cnolib_amd64.o cnolib.o shnolib.o
	ld --gc-sections -s -o $(TARGET) cnolib.o cnolib_amd64.o shnolib.o

clean:
	rm -f *.o $(TARGET) foo*

