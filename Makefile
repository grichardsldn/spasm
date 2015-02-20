CFLAGS=-g

all: spasm

clean:
	rm *.o spasm

spasm: spasm.o text.o 
	${CC} -o spasm spasm.o text.o



