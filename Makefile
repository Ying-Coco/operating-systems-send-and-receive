CXX_FLAGS=-Wall -Wextra -g
CXX=gcc
BIN=$(CXX) $(CXX_FLAGS)

compile: clean recv sender

recv:
	$(BIN) recv.c -o recv

sender:
	$(BIN) send.c -o sender

uncrustify:
	uncrustify --replace --no-backup -c style.cfg *.c

clean:
	rm -f sender recv a.out core.*

.PHONY: compile
