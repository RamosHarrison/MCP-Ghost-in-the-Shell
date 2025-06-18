CC = gcc
CFLAGS = -Wall -Wextra -g
TARGETS = part1 part2 part3 part4 part5 

all: $(TARGETS)

part1: part1.c
	$(CC) $(CFLAGS) -o part1 part1.c

part2: part2.c
	$(CC) $(CFLAGS) -o part2 part2.c

part3: part3.c
	$(CC) $(CFLAGS) -o part3 part3.c

part4: part4.c
	$(CC) $(CFLAGS) -o part4 part4.c

part5: part5.c
	$(CC) $(CFLAGS) -o part5 part5.c


clean:
	rm -f $(TARGETS) *.o *~ core

valgrind-%: %
	valgrind --leak-check=full --track-origins=yes ./$(*) $(ARGS) > log_$*.txt 2>&1
