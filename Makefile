FLAGS=$(-std=gnu90)

analyzer.o: ./src/analyzer/analyzer.c ./src/analyzer/analyzer.h
	gcc $(FLAGS) -c ./src/analyzer/analyzer.c

manager.o: ./src/manager/manager.c ./src/manager/manager.h
	gcc $(FLAGS) -c ./src/manager/manager.c

reporter.o: ./src/reporter/reporter.c ./src/reporter/reporter.h
	gcc $(FLAGS) -c ./src/reporter/reporter.c

worker.o: ./src/worker/worker.c ./src/worker/worker.h
	gcc $(FLAGS) -c ./src/worker/worker.c

wrapping.o: ./src/wrapping/wrapping.c ./src/wrapping/wrapping.h
	gcc $(FLAGS) -c ./src/wrapping/wrapping.c

main.o: ./src/main.c \
	./src/analyzer/analyzer.h \
	./src/manager/manager.h \
	./src/reporter/reporter.h \
	./src/worker/worker.h \
	./src/wrapping/wrapping.h
	gcc $(FLAGS) -c ./src/main.c

.SILENT:
help:
	cat README

build: main.o \
	analyzer.o \
	manager.o \
	reporter.o \
	worker.o \
	wrapping.o
	gcc $(FLAGS) -o counter \
		main.o \
		analyzer.o \
		manager.o \
		reporter.o \
		worker.o \
		wrapping.o

clean:
	rm counter \
		main.o \
		analyzer.o \
		manager.o \
		reporter.o \
		worker.o \
		wrapping.o
