FLAGS=-std=gnu90
FLAGS_THREAD=-pthread
BIN_FOLDER=./bin/

$(BIN_FOLDER)analyzer.o: ./src/analyzer/analyzer.c \
	./src/analyzer/analyzer.h \
	./src/manager/manager.h \
	./src/list/list.h \
	./src/priorityQueue/priorityQueue.h \
	./src/table/table.h \
	./src/tree/tree.h \
	./src/wrapping/wrapping.h 
	gcc $(FLAGS) $(FLAGS_THREAD) -c ./src/analyzer/analyzer.c -o $(BIN_FOLDER)analyzer.o

$(BIN_FOLDER)manager.o: ./src/manager/manager.c \
	./src/manager/manager.h \
	./src/list/list.h \
	./src/wrapping/wrapping.h \
	./src/worker/worker.h \
	./src/work/work.h
	gcc $(FLAGS) $(FLAGS_THREAD) -c ./src/manager/manager.c -o $(BIN_FOLDER)manager.o

$(BIN_FOLDER)reporter.o: ./src/reporter/reporter.c \
	./src/reporter/reporter.h \
	./src/list/list.c
	gcc $(FLAGS) $(FLAGS_THREAD) -c ./src/reporter/reporter.c -o $(BIN_FOLDER)reporter.o

$(BIN_FOLDER)list.o: ./src/list/list.c ./src/list/list.h
	gcc $(FLAGS) -c ./src/list/list.c -o $(BIN_FOLDER)list.o

$(BIN_FOLDER)priorityQueue.o: ./src/priorityQueue/priorityQueue.c ./src/priorityQueue/priorityQueue.h
	gcc $(FLAGS) -c ./src/priorityQueue/priorityQueue.c -o $(BIN_FOLDER)priorityQueue.o

$(BIN_FOLDER)table.o: ./src/table/table.c ./src/table/table.h
	gcc $(FLAGS) -c ./src/table/table.c -o $(BIN_FOLDER)table.o

$(BIN_FOLDER)tree.o: ./src/tree/tree.c \
	./src/list/list.h \
	./src/tree/tree.h
	gcc $(FLAGS) -c ./src/tree/tree.c -o $(BIN_FOLDER)tree.o

$(BIN_FOLDER)tui.o: ./src/tui/tui.c ./src/tui/tui.h
	gcc $(FLAGS) $(FLAGS_THREAD) -c ./src/tui/tui.c -o $(BIN_FOLDER)tui.o

$(BIN_FOLDER)work.o: ./src/work/work.c ./src/work/work.h
	gcc $(FLAGS) -c ./src/work/work.c -o $(BIN_FOLDER)work.o

$(BIN_FOLDER)worker.o: ./src/worker/worker.c ./src/worker/worker.h
	gcc $(FLAGS) -c ./src/worker/worker.c  -o $(BIN_FOLDER)worker.o

$(BIN_FOLDER)wrapping.o: ./src/wrapping/wrapping.c ./src/wrapping/wrapping.h
	gcc $(FLAGS) -c ./src/wrapping/wrapping.c  -o $(BIN_FOLDER)wrapping.o

$(BIN_FOLDER)main.o: ./src/main.c
	gcc $(FLAGS) -c ./src/main.c -o $(BIN_FOLDER)main.o

.SILENT:
help:
	cat README

makeDir:
	[ -d bin ] || mkdir bin

build: makeDir \
	clean \
	$(BIN_FOLDER)main.o \
  $(BIN_FOLDER)analyzer.o \
  $(BIN_FOLDER)manager.o \
  $(BIN_FOLDER)reporter.o \
  $(BIN_FOLDER)list.o \
  $(BIN_FOLDER)priorityQueue.o \
  $(BIN_FOLDER)table.o \
  $(BIN_FOLDER)tree.o \
  $(BIN_FOLDER)tui.o \
  $(BIN_FOLDER)work.o \
  $(BIN_FOLDER)worker.o \
  $(BIN_FOLDER)wrapping.o
	echo "building..." 
	gcc $(FLAGS) $(FLAGS_THREAD) -o $(BIN_FOLDER)analyzer \
	  $(BIN_FOLDER)analyzer.o \
	  $(BIN_FOLDER)list.o \
	  $(BIN_FOLDER)priorityQueue.o \
	  $(BIN_FOLDER)table.o \
	  $(BIN_FOLDER)tree.o \
	  $(BIN_FOLDER)wrapping.o
	gcc $(FLAGS) -o $(BIN_FOLDER)counter \
	  $(BIN_FOLDER)main.o \
	  $(BIN_FOLDER)list.o \
	  $(BIN_FOLDER)work.o \
	  $(BIN_FOLDER)wrapping.o
	gcc $(FLAGS) -o $(BIN_FOLDER)worker \
      $(BIN_FOLDER)table.o \
      $(BIN_FOLDER)worker.o \
      $(BIN_FOLDER)wrapping.o
	gcc $(FLAGS) $(FLAGS_THREAD) -o $(BIN_FOLDER)manager \
	  $(BIN_FOLDER)manager.o \
	  $(BIN_FOLDER)list.o \
	  $(BIN_FOLDER)table.o \
	  $(BIN_FOLDER)wrapping.o \
	  $(BIN_FOLDER)work.o
	gcc $(FLAGS) $(FLAGS_THREAD) -o $(BIN_FOLDER)reporter \
	  $(BIN_FOLDER)list.o \
	  $(BIN_FOLDER)reporter.o \
	  $(BIN_FOLDER)wrapping.o \
		$(BIN_FOLDER)tui.o
	gcc $(FLAGS) $(FLAGS_THREAD) -o $(BIN_FOLDER)counter\
	  $(BIN_FOLDER)main.o \
	  $(BIN_FOLDER)wrapping.o
	echo "done"

log: build \
	$(BIN_FOLDER)main.o \
  $(BIN_FOLDER)wrapping.o
	echo "adding log option..."
	gcc $(FLAGS) $(FLAGS_THREAD) -D LOG -o $(BIN_FOLDER)counter\
	  ./src/main.c \
	  $(BIN_FOLDER)wrapping.o
	echo "done"

clean:
	rm -rf $(BIN_FOLDER)*

cleanObj:
	rm -rf $(BIN_FOLDER)*.o

buildMinimal: build \
	cleanObj
