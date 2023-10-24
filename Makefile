
CC = gcc -pipe
CFLAGS = -g -O2 -std=gnu11 -Wall -W -Wextra -Wshadow -Winline -Werror
CPPFLAGS = -D_FILE_OFFSET_BITS=64

all: fontx2psf

fontx2psf: fontx2psf.c

clean:
	rm -f fontx2psf
