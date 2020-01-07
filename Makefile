SRC = jimmy_fallon.c
OBJ = jimmy_fallon.o
PROG = jimmy_fallon

$(PROG) : $(OBJ)
	gcc $(OBJ) -pthread -o $(PROG)

$(OBJ): $(SRC)
