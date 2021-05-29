obj = main.o queue.o
OBJRS = readers-starvation.o queue.o
OBJWR = writers-starvation.o queue.o
APP = rwp
FLAGS = -pthread -lrt -Wextra -Wpedantic -Wall

makea: $(obj)
	gcc -o $(APP) $(obj) $(FLAGS)

readers-starvation: $(OBJRS)
	gcc -o r-starvation $(OBJRS) $(FLAGS)
	
writers-starvation: $(OBJWR)
	gcc -o w-starvation $(OBJWR) $(FLAGS)

main.o: queue.h
	cc -c -o main.o main.c
queue.o: queue.h
	cc -c -o queue.o queue.c
	
.PHONY: clean
clean:
	rm -rf *.o

rebuild:
	$(MAKE) clean
	$(MAKE) $(param1)
