CC = tcc
OUT = lwm
PREFIX = /usr/local

all:
	${CC} -O2 -o ${OUT} *.c -lX11

install: all
	mkdir -p ${PREFIX}/bin
	install -s ${OUT} ${PREFIX}/bin

uninstall:
	rm ${PREFIX}/bin/${OUT}

.PHONY: all install uninstall
