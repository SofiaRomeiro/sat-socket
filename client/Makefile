CC = gcc
CFLAGS = -Wall -lm
INCLUDES ?= -I./

EXEC = client.exe
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

%.o: %.c
	$(CC) $(INCLUDES) -c $< -o $@	$(CFLAGS)	

all: $(OBJS)
	$(CC) $(INCLUDES) $(OBJS) -o $(EXEC) $(CFLAGS)	

.PHONY : clean
clean:
	$(RM) *.o *~  $(EXEC)
