CC = gcc
CFLAGS = -Wall -g

OUT = ftp_client
SRC = client.c
OBJS = $(SRC:.c=.o)

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $^ -o $@
	rm -f $^

$(OBJS): $(SRC)
	$(CC) $(CFLAGS) -c $^

clean:
	rm -f $(OBJS) $(OUT)