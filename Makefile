OBJ = main.o queue.o
OBJRS = readers-starvation.o queue.o
OBJWR = writers-starvation.o queue.o
APP = rwp
APP2 = rstar
APP3= wstar
FLAGS = -pthread -lrt -Wextra -Wpedantic -Wall

default: # Domyślny target
	@echo "Brak określonego celu budowania lub uruchomienia. Dostępne są możliwości:"
	@echo "    make final			\t- Budowa pliku wykonywalnego dla finalnej wersji"
	@echo "    make rebuild target		\t- Przebudowa pliku wykonywalnego"
	@echo "    make clean			\t- Usunięcie pliku wykonywalnego i plików pośrenidch"
	@echo "    make readers-starvation	\t- Budowa pliku wykonywalnego z przykładem zagłodzenia czytelników"
	@echo "    make writers-starvation	\t- Budowa pliku wykonywalnego z przykładem zagłodzenia pisarzy"
	@echo "    make ticket-lock		\t- Budowa pliku wykonywalnego z przykładem mechanizmu ticketlock"
	@echo ""

final: $(OBJ)
	@echo "Building correct solution"
	@echo "Name of app: $(APP)"
	gcc -o $(APP) $(OBJ) $(FLAGS)

readers-starvation: $(OBJRS)
	@echo "Building readers starvation example"
	@echo "Name of app: $(APP2)"
	gcc -o $(APP2) $(OBJRS) $(FLAGS)
	
writers-starvation: $(OBJWR)
	@echo "Bulding writers starvation example"
	@echo "Name of app: $(APP3)"
	gcc -o $(APP3) $(OBJWR) $(FLAGS)

ticket-lock: ticket-lock.o queue.o
	@echo "Bulding ticket-lock example"
	gcc -o ticketlock ticket-lock.o queue.o $(FLAGS)

main.o: queue.h
	cc -c -o main.o main.c
queue.o: queue.h
	cc -c -o queue.o queue.c
	
.PHONY: clean
clean:
	rm -rf *.o $(APP) $(APP2) $(APP3)

rebuild:
	$(MAKE) clean
	$(MAKE) $(target)
