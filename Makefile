OBJ=kernel 

all: $(OBJ) 

kernel: 
	make -C kern
	make -C userland

clean:
	rm -f $(OBJ) *.o bochs.log
	make -C kern clean
	make -C userland clean
