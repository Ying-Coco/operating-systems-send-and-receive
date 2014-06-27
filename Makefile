CXX_FLAGS=-Wall -Wextra
CXX=gcc
BIN=$(CXX) $(CXX_FLAGS)

uncrustify:
	uncrustify --replace --no-backup -c style.cfg *.c

recv:
	$(BIN) recv.c -o recv

send:
	$(BIN) send.c -o send
