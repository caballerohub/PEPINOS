OBJ=libc.a
CC=gcc -m32 -fno-pie -fno-stack-protector -fcommon -Wall -fno-builtin

all: $(OBJ) 

libc.a: libc.o syscalls.o malloc.o
	ar rcs $@ $^

.o: .c 
	$(CC) -c $^

clean:
	rm -f $(OBJ) *.o *.a
