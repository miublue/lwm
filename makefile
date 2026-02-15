CC = tcc
OUT = lwm
LIBS = -lX11
PREFIX = /usr/local
CFLAGS = -Wall -Werror

all:
	${CC} -O2 -o ${OUT} *.c -I. ${CFLAGS} ${LIBS}

install: all
	mkdir -p ${PREFIX}/bin
	install -s ${OUT} ${PREFIX}/bin

uninstall:
	rm ${PREFIX}/bin/${OUT}

.PHONY: all install uninstall
