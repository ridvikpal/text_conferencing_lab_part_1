CC = gcc
CFLAGS = -I.
LIBS = -lpthread

DEPS = message.h session.h user.h
OBJ = client.o server.o message.o session.o user.o

TARGETS = server client

all: $(TARGETS)

server: server.o message.o session.o user.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

client: client.o message.o session.o user.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGETS) *.o
