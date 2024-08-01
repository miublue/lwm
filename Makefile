OUT = lwm

all:
	tcc -o $(OUT) *.c -lX11 -O2

install: all
	install $(OUT) /usr/local/bin

uninstall:
	rm /usr/local/bin/$(OUT)

