
all: example

clean:
	rm -f *.o example

example: jay.o example.o

.PHONY: all clean
