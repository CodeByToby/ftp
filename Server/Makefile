CC = gcc
CFLAGS = -Wall -g

OUT = ftp_server
SRC = server.c handlers.c
OBJS = $(SRC:.c=.o)

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $^ -o $@
	rm -f $^

$(OBJS): $(SRC)
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(OUT)