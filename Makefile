CC = gcc
CFLAGS += -I/usr/src/linux-2.4.18-14custom/include -Wall
OBJS = pubsub.o

all: $(OBJS)
    
clean:
	rm -f *.o *~
