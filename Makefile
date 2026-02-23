
CC = gcc -pipe
CFLAGS = -g -O2 -std=gnu11 -Wall -W -Wextra -Wshadow -Winline -Werror

all: fontx2psf

fontx2psf: fontx2psf.c

clean:
	rm -f fontx2psf
