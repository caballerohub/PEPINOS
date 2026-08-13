OBJ=shell cat kill task1 task2
CC=gcc -m32 -fno-pie -fno-stack-protector -fcommon -Wall -fno-builtin

all: $(OBJ) 

shell: shell.o libc/libc.a 
	ld -m elf_i386 -n -Ttext=40000000 --entry=main -o $@ $^

cat: cat.o libc/libc.a
	ld -m elf_i386 -n -Ttext=40000000 --entry=main -o $@ $^

kill: kill.o libc/libc.a
	ld -m elf_i386 -n -Ttext=40000000 --entry=main -o $@ $^

task1: task1.o libc/libc.a
	ld -m elf_i386 -n -Ttext=40000000 --entry=main -o $@ $^

task2: task2.o libc/libc.a
	ld -m elf_i386 -n -Ttext=40000000 --entry=main -o $@ $^

libc/libc.a: 
	make -C libc

.o: .c 
	$(CC) -c $^

clean:
	rm -f $(OBJ) *.o *.a
	make -C libc clean
