CXX_FLAGS=-Wall -Wextra -g
CXX=gcc
BIN=$(CXX) $(CXX_FLAGS)

uncrustify:
	uncrustify --replace --no-backup -c style.cfg *.c

compile: clean recv send

recv:
	$(BIN) recv.c -o recv

send:
	$(BIN) send.c -o send

clean:
	rm -f send recv a.out core.*

.PHONY: compile
