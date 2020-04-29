FLAGS=$(-std=gnu90)
wrapping.o: ./src/wrapping.c ./src/wrapping.h
	gcc $(FLAGS) -c ./src/wrapping.c

main.o: ./src/main.c ./src/wrapping.h
	gcc $(FLAGS) -c ./src/main.c

.SILENT:
help:
	cat README

build: main.o wrapping.o
	gcc $(FLAGS) -o counter \
		main.o \
		wrapping.o

clean:
	rm counter \
		main.o \
		wrapping.o
