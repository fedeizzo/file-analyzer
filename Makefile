FLAGS=$(-std=gnu90 -pthread)

analyzer.o: ./src/analyzer/analyzer.c ./src/analyzer/analyzer.h
	gcc $(FLAGS) -c ./src/analyzer/analyzer.c

manager.o: ./src/manager/manager.c ./src/manager/manager.h
	gcc $(FLAGS) -c ./src/manager/manager.c

reporter.o: ./src/reporter/reporter.c ./src/reporter/reporter.h
	gcc $(FLAGS) -c ./src/reporter/reporter.c

queue.o: ./src/queue/queue.c ./src/queue/queue.h
	gcc $(FLAGS) -c ./src/queue/queue.c

work.o: ./src/work/work.c ./src/work/work.h
	gcc $(FLAGS) -c ./src/work/work.c

worker.o: ./src/worker/worker.c ./src/worker/worker.h
	gcc $(FLAGS) -c ./src/worker/worker.c

wrapping.o: ./src/wrapping/wrapping.c ./src/wrapping/wrapping.h
	gcc $(FLAGS) -c ./src/wrapping/wrapping.c

main.o: ./src/main.c \
	./src/analyzer/analyzer.h \
	./src/manager/manager.h \
	./src/reporter/reporter.h \
	./src/queue/queue.h \
	./src/work/work.h \
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
	queue.o \
	work.o \
	worker.o \
	wrapping.o
	gcc $(FLAGS) -o counter \
		main.o \
		analyzer.o \
		manager.o \
		reporter.o \
		queue.o \
		work.o \
		wrapping.o
	gcc $(FLAGS) -o worker \
		worker.o \
		wrapping.o

clean:
	rm counter \
		worker \
		main.o \
		analyzer.o \
		manager.o \
		reporter.o \
		queue.o \
		work.o \
		worker.o \
		wrapping.o
