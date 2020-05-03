FLAGS=-std=gnu90
BIN_FOLDER=./bin/

$(BIN_FOLDER)analyzer.o: ./src/analyzer/analyzer.c ./src/analyzer/analyzer.h
	gcc $(FLAGS) -c ./src/analyzer/analyzer.c -o $(BIN_FOLDER)analyzer.o

$(BIN_FOLDER)manager.o: ./src/manager/manager.c \
	./src/manager/manager.h \
	./src/list/list.h	\
	./src/wrapping/wrapping.h \
	./src/worker/worker.h \
	./src/work/work.h
	gcc $(FLAGS) -c ./src/manager/manager.c -o $(BIN_FOLDER)manager.o

$(BIN_FOLDER)reporter.o: ./src/reporter/reporter.c ./src/reporter/reporter.h
	gcc $(FLAGS) -c ./src/reporter/reporter.c -o $(BIN_FOLDER)reporter.o

$(BIN_FOLDER)list.o: ./src/list/list.c ./src/list/list.h
	gcc $(FLAGS) -c ./src/list/list.c -o $(BIN_FOLDER)list.o

$(BIN_FOLDER)table.o: ./src/table/table.c ./src/table/table.h
	gcc $(FLAGS) -c ./src/table/table.c -o $(BIN_FOLDER)table.o

$(BIN_FOLDER)work.o: ./src/work/work.c ./src/work/work.h
	gcc $(FLAGS) -c ./src/work/work.c -o $(BIN_FOLDER)work.o

$(BIN_FOLDER)worker.o: ./src/worker/worker.c ./src/worker/worker.h
	gcc $(FLAGS) -c ./src/worker/worker.c  -o $(BIN_FOLDER)worker.o

$(BIN_FOLDER)wrapping.o: ./src/wrapping/wrapping.c ./src/wrapping/wrapping.h
	gcc $(FLAGS) -c ./src/wrapping/wrapping.c  -o $(BIN_FOLDER)wrapping.o

$(BIN_FOLDER)main.o: ./src/main.c \
	./src/analyzer/analyzer.h \
	./src/manager/manager.h \
	./src/reporter/reporter.h \
	./src/list/list.h \
	./src/table/table.h \
	./src/work/work.h \
	./src/worker/worker.h \
	./src/wrapping/wrapping.h
	gcc $(FLAGS) -c ./src/main.c -o $(BIN_FOLDER)main.o

help:
	cat README

.SILENT:
build: $(BIN_FOLDER)main.o \
  $(BIN_FOLDER)analyzer.o \
  $(BIN_FOLDER)manager.o \
  $(BIN_FOLDER)reporter.o \
  $(BIN_FOLDER)list.o \
  $(BIN_FOLDER)table.o \
  $(BIN_FOLDER)work.o \
  $(BIN_FOLDER)worker.o \
  $(BIN_FOLDER)wrapping.o
	gcc $(FLAGS) -o $(BIN_FOLDER)counter \
	  $(BIN_FOLDER)main.o \
	  $(BIN_FOLDER)analyzer.o \
	  $(BIN_FOLDER)reporter.o \
	  $(BIN_FOLDER)list.o \
	  $(BIN_FOLDER)work.o \
	  $(BIN_FOLDER)wrapping.o
	gcc $(FLAGS) -o $(BIN_FOLDER)worker \
    $(BIN_FOLDER)table.o \
    $(BIN_FOLDER)worker.o \
    $(BIN_FOLDER)wrapping.o
	gcc $(FLAGS) -o $(BIN_FOLDER)manager \
		$(BIN_FOLDER)manager.o \
	  $(BIN_FOLDER)list.o \
	  $(BIN_FOLDER)table.o \
	  $(BIN_FOLDER)wrapping.o \
	  $(BIN_FOLDER)work.o

clean:
	rm $(BIN_FOLDER)counter \
	  $(BIN_FOLDER)worker \
	  $(BIN_FOLDER)manager \
	  $(BIN_FOLDER)main.o \
	  $(BIN_FOLDER)analyzer.o \
	  $(BIN_FOLDER)manager.o \
	  $(BIN_FOLDER)reporter.o \
	  $(BIN_FOLDER)list.o \
	  $(BIN_FOLDER)table.o \
	  $(BIN_FOLDER)work.o \
	  $(BIN_FOLDER)worker.o \
	  $(BIN_FOLDER)wrapping.o
