CC = /usr/local/xenon/bin/xenon-gcc
OBJCOPY = /usr/local/xenon/bin/xenon-objcopy

CFLAGS = -O3 -I/usr/local/xenon/usr/include
LDFLAGS = -L/usr/local/xenon/usr/lib -lxenon -lm

all: horse_game.elf

horse_game.elf: main.o
	$(CC) main.o $(LDFLAGS) -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f *.o *.elf *.xex
