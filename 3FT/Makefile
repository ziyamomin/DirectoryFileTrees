# Makefile for Part 3: File Tree

# Compiler and flags
CC = gcc217
CFLAGS = -g -Wall -std=c99

# Object files
OBJS = ft.o nodeFT.o path.o dynarray.o ft_client.o

# Executable name
EXEC = ft

# Default target
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(EXEC)

# Compile each .c to .o
ft.o: ft.c ft.h nodeFT.h a4def.h
	$(CC) $(CFLAGS) -c ft.c

nodeFT.o: nodeFT.c nodeFT.h path.h dynarray.h a4def.h
	$(CC) $(CFLAGS) -c nodeFT.c

path.o: path.c path.h a4def.h
	$(CC) $(CFLAGS) -c path.c

dynarray.o: dynarray.c dynarray.h
	$(CC) $(CFLAGS) -c dynarray.c

ft_client.o: ft_client.c ft.h
	$(CC) $(CFLAGS) -c ft_client.c

# Clean target to remove object files and executable
clean:
	rm -f *.o $(EXEC)
