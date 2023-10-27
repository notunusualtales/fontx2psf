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

#define CHARLINE_BYTES(w) (((w) + 7) / 8)

#define PSF_VERSION_1 (1)
#define PSF_VERSION_2 (2)

#define MARGIN_MIN (0)
#define MARGIN_MAX (8)

void read_fontx_header(int fd, struct fontx_header *header)
{
	ssize_t sz = sizeof(struct fontx_header);

	if (read(fd, header, sz) != sz) {
		fprintf(stderr, "ERROR: Unable to read fontx header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

void write_psf1_header(int fd, uint32_t margin, struct fontx_header *header)
{
	uint32_t height = header->height + margin;

	struct psf1_header h = {
		.magic    = { PSF1_MAGIC0, PSF1_MAGIC1 },
		.mode     = 0,
		.charsize = height * CHARLINE_BYTES(header->width)
	};

	ssize_t sz = sizeof(struct psf1_header);

	if (write(fd, &h, sz) != sz) {
		fprintf(stderr, "ERROR: Unable to write psf1 header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

void write_psf2_header(int fd, uint32_t margin, struct fontx_header *header)
{
	uint32_t height = header->height + margin;

	struct psf2_header h = {
		.magic      = { PSF2_MAGIC0, PSF2_MAGIC1, PSF2_MAGIC2, PSF2_MAGIC3 },
		.version    = 0,
		.headersize = sizeof(struct psf2_header),
		.flags      = 0,
		.length     = PSF2_CODEBLOCKS,
		.charsize   = height * CHARLINE_BYTES(header->width),
		.height     = height,
		.width      = header->width
	};

	if (write(fd, &h, h.headersize) != h.headersize) {
		fprintf(stderr, "ERROR: Unable to write psf2 header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

int copy_data(int in, int out, uint32_t margin, struct fontx_header *header)
{
	char buf[BUFSIZ];
	memset(buf, 0, BUFSIZ);

	uint32_t top = margin / 2;
	uint32_t bottom = (margin + 1) / 2;

	uint32_t cb = CHARLINE_BYTES(header->width);

	uint32_t wh = cb * header->height;
	uint32_t wt = cb * top;
	uint32_t mb = cb * bottom + wt;

	for (;;) {
		ssize_t sz = read(in, &buf[wt], wh);

		if (sz > 0) {
			sz += mb;

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

void usage(FILE *file, char *name)
{
	fprintf(file, "Usage: %s [-p psfver] [-m margin]\n", name);
	fprintf(file, "    psfver: %d or %d (default: %d)\n", PSF_VERSION_1, PSF_VERSION_2, PSF_VERSION_2);
	fprintf(file, "    margin: %d - %d (default: %d)\n", MARGIN_MIN, MARGIN_MAX, MARGIN_MIN);
}

void parse(int argc, char *argv[], uint32_t *psfver, uint32_t *margin)
{
	char *name = argv[0];

	*psfver = PSF_VERSION_2;
	*margin = MARGIN_MIN;

	int opt = 0;

	optarg = NULL;
	optind = 1;
	opterr = 1;
	optopt = 0;

	while ((opt = getopt(argc, argv, "p:m:h")) != -1) {
		switch (opt) {
		case 'p':
			int p = atoi(optarg);
			if (p == PSF_VERSION_1 || p == PSF_VERSION_2) {
				*psfver = p;
				break;
			} else {
				fprintf(stderr, "%s: invalid parameter: -p [%s]\n", name, optarg);
				exit(EXIT_FAILURE);
			}
		case 'm':
			int m = atoi(optarg);
			if (MARGIN_MIN <= m && m <= MARGIN_MAX) {
				*margin = m;
				break;
			} else {
				fprintf(stderr, "%s: invalid parameter: -m [%s]\n", name, optarg);
				exit(EXIT_FAILURE);
			}
		case 'h':
			usage(stdout, name);
			exit(EXIT_SUCCESS);

		default:
			usage(stderr, name);
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char *argv[])
{
	uint32_t psfver = PSF_VERSION_2;
	uint32_t margin = MARGIN_MIN;

	parse(argc, argv, &psfver, &margin);

	struct fontx_header fontx_header = {};

	read_fontx_header(STDIN_FILENO, &fontx_header);

	if (psfver == PSF_VERSION_1) {
		write_psf1_header(STDOUT_FILENO, margin, &fontx_header);
	} else {
		write_psf2_header(STDOUT_FILENO, margin, &fontx_header);
	}

	copy_data(STDIN_FILENO, STDOUT_FILENO, margin, &fontx_header);

	return 0;
}
