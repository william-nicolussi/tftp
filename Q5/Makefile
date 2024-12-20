SRC = $(wildcard *.c)
BIN = $(SRC:.c=)
CFLAGS += -Wall -Wextra -g

NAME_GET = gettftp
NAME_PUT = puttftp

all: #$(BIN)

indent:
	indent -linux -i4 -nut -ts2 *.c

clean:
	rm -f $(BIN)
	rm -f *.o

.PHONY: all clean indent

put:
	gcc $(NAME_PUT).c -o $(NAME_PUT) $(CFLAGS)
