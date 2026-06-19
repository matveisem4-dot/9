CC = powerpc-linux-gnu-gcc
OBJCOPY = powerpc-linux-gnu-objcopy

CFLAGS = -O3 -m32 -fno-pic -mno-altivec -ffreestanding -nostdlib -ffunction-sections -fdata-sections
LDFLAGS = -nostartfiles -nodefaultlibs -Wl,--gc-sections -Ttext=0x80000000

all: horse_game.elf

horse_game.elf: main.o
	$(CC) $(CFLAGS) $(LDFLAGS) main.o -o $@

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

clean:
	rm -f *.o *.elf
