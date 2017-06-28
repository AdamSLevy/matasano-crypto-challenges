CFLAGS=-Wall -O3 -s
LDFLAGS=-Wall -O3 -s

all: single-byte-xor-cipher-crack 	\
     fixed-xor 				\
     hex-to-b64 			\
     hex-to-char 			\
     b64-to-hex 			\
     b64-to-char

single-byte-xor-cipher-crack: single-byte-xor-cipher-crack.o raw.o frequency-analysis.o
single-byte-xor-cipher-crack.o: single-byte-xor-cipher-crack.c raw.h frequency-analysis.h

fixed-xor:   fixed-xor.o   raw.o
fixed-xor.o:   fixed-xor.c   raw.h

hex-to-b64:  hex-to-b64.o  raw.o
hex-to-char: hex-to-char.o raw.o
b64-to-hex:  b64-to-hex.o  raw.o
b64-to-char: b64-to-char.o raw.o

hex-to-b64.o:  hex-to-b64.c  raw.h
hex-to-char.o: hex-to-char.c raw.h
b64-to-hex.o:  b64-to-hex.c  raw.h
b64-to-char.o: b64-to-char.c raw.h

raw.o: raw.c raw.h
frequency-analysis.o: frequency-analysis.c frequency-analysis.h

clean: 
	rm hex-to-b64 hex-to-char b64-to-hex b64-to-char *.o