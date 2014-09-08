CFLAGS = -c -O3
MODEL = -mthumb -mthumb-interwork

all: main.gba

main.gba: main.elf
	objcopy -O binary main.elf main.gba

main.elf: main.o
	gcc $(MODEL) -o main.elf main.o

main.o: main.c
	gcc $(CFLAGS) $(MODEL) main.c
