CC = gcc
CFLAGS = -I.

DEPS = message.h session.h user.h
TARGET = server client message session user

all: ${TARGET}

server: server.c ${DEPS}
		gcc -o server server.c ${CFLAGS}

client: client.c ${DEPS}
		gcc -o client client.c ${CFLAGS}

message: message.c ${DEPS}
		gcc -o message message.c ${CFLAGS}

session: session.c ${DEPS}
		gcc -o session session.c ${CFLAGS}

session: user.c ${DEPS}
		gcc -o user user.c ${CFLAGS}

clean:
		rm -f ${TARGET}