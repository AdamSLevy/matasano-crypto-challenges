CFLAGS=-Wall -O3 -s
LDFLAGS=-Wall -O3 -s

all: detect-aes-128-ecb			\
     decrypt-aes-128-ecb		\
     decrypt-aes-128-cbc		\
     convert 				\
     compute-hamming-distance 		\
     repeating-key-xor-crack 		\
     repeating-key-xor			\
     single-byte-xor-cipher-crack 	\
     fixed-xor

detect-aes-128-ecb: detect-aes-128-ecb.o raw.o hamming-distance.o sorted-list.o
detect-aes-128-ecb.o: detect-aes-128-ecb.c raw.h hamming-distance.h sorted-list.h

decrypt-aes-128-ecb: LDFLAGS+= -lcrypto
decrypt-aes-128-ecb: decrypt-aes-128-ecb.o raw.o
decrypt-aes-128-ecb.o: decrypt-aes-128-ecb.c raw.h

decrypt-aes-128-cbc: LDFLAGS+= -lcrypto
decrypt-aes-128-cbc: decrypt-aes-128-cbc.o raw.o
decrypt-aes-128-cbc.o: decrypt-aes-128-cbc.c raw.h

convert: convert.o raw.o convert_argp.o
convert.o: convert.c raw.h convert_argp.h
convert_argp.o: convert_argp.c convert_argp.h

repeating-key-xor-crack:   repeating-key-xor-crack.o \
    raw.o \
    hamming-distance.o \
    frequency-analysis.o
repeating-key-xor-crack.o: repeating-key-xor-crack.c \
    raw.h \
    hamming-distance.h \
    frequency-analysis.h

compute-hamming-distance:   compute-hamming-distance.o raw.o hamming-distance.o
compute-hamming-distance.o: compute-hamming-distance.c raw.h hamming-distance.h

repeating-key-xor: repeating-key-xor.o raw.o
repeating-key-xor.o: repeating-key-xor.c raw.h

single-byte-xor-cipher-crack: single-byte-xor-cipher-crack.o raw.o frequency-analysis.o
single-byte-xor-cipher-crack.o: single-byte-xor-cipher-crack.c raw.h frequency-analysis.h

fixed-xor:   fixed-xor.o   raw.o
fixed-xor.o:   fixed-xor.c   raw.h

raw.o: raw.c raw.h
frequency-analysis.o: frequency-analysis.c frequency-analysis.h
hamming-distance.o: hamming-distance.c raw.h
sorted-list.o: sorted-list.c sorted-list.h

clean: 
	rm *.o					\
	 compute-hamming-distance 		\
	 repeating-key-xor-crack 		\
	 repeating-key-xor			\
	 single-byte-xor-cipher-crack 		\
	 fixed-xor
