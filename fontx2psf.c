/*
 *  fontx2psf.c
 *
 *  Copyright (C) 2023 Not Unusual Tales
 *
 *  SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define FONTX_CODEBLOCKS (256)

struct fontx_header {
	char signature[6];
	char fontname[8];
	uint8_t width;
	uint8_t height;
	uint8_t codeflag;
};

#define PSF1_MAGIC0     (0x36)
#define PSF1_MAGIC1     (0x04)

struct psf1_header {
	uint8_t magic[2];
	uint8_t mode;
	uint8_t charsize;
};

#define PSF2_MAGIC0 (0x72)
#define PSF2_MAGIC1 (0xb5)
#define PSF2_MAGIC2 (0x4a)
#define PSF2_MAGIC3 (0x86)

#define PSF2_CODEBLOCKS FONTX_CODEBLOCKS

struct psf2_header {
	uint8_t magic[4];
	uint32_t version;
	uint32_t headersize;
	uint32_t flags;
	uint32_t length;
	uint32_t charsize;
	uint32_t height;
	uint32_t width;
};

void read_fontx_header(int fd, struct fontx_header *header)
{
	ssize_t sz = sizeof(struct fontx_header);

	if (read(fd, header, sz) != sz) {
		fprintf(stderr, "ERROR: Unable to read fontx header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

void write_psf1_header(int fd, struct fontx_header *header)
{
	struct psf1_header h = {
		.magic    = { PSF1_MAGIC0, PSF1_MAGIC1 },
		.mode     = 0,
		.charsize = header->height
	};

	ssize_t sz = sizeof(struct psf1_header);

	if (write(fd, &h, sz) != sz) {
		fprintf(stderr, "ERROR: Unable to write psf1 header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

void write_psf2_header(int fd, struct fontx_header *header)
{
	struct psf2_header h = {
		.magic      = { PSF2_MAGIC0, PSF2_MAGIC1, PSF2_MAGIC2, PSF2_MAGIC3 },
		.version    = 0,
		.headersize = sizeof(struct psf2_header),
		.flags      = 0,
		.length     = PSF2_CODEBLOCKS,
		.charsize   = header->height * ((header->width + 7) / 8),
		.height     = header->height,
		.width      = header->width
	};

	if (write(fd, &h, h.headersize) != h.headersize) {
		fprintf(stderr, "ERROR: Unable to write psf2 header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

int copy_data(int in, int out)
{
	char buf[BUFSIZ];

	for (;;) {
		ssize_t sz = read(in, buf, BUFSIZ);

		if (sz > 0) {
			if (write(out, buf, sz) != sz) {
				fprintf(stderr, "ERROR: Unable to write psf data: %s\n",
					errno ? strerror(errno) : "(Unknown)");
				exit(EXIT_FAILURE);
			}

		} else if (sz == 0) {
			return 0;

		} else {
			fprintf(stderr, "ERROR: Unable to read fontx data: %s\n",
				errno ? strerror(errno) : "(Unknown)");
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char *argv[])
{
	struct fontx_header fontx_header = {};

	read_fontx_header(STDIN_FILENO, &fontx_header);

	if (argc > 1 && *argv[1] == '1') {
		write_psf1_header(STDOUT_FILENO, &fontx_header);
	} else {
		write_psf2_header(STDOUT_FILENO, &fontx_header);
	}

	copy_data(STDIN_FILENO, STDOUT_FILENO);

	return 0;
}
