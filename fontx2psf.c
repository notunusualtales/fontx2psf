/*
 *  fontx2psf.c
 *
 *  Copyright (C) 2023-2026 Not Unusual Tales
 *
 *  SPDX-License-Identifier: MIT
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
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

#define PSF1_MAGIC0 (0x36)
#define PSF1_MAGIC1 (0x04)

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

struct font_info {
	uint32_t psfver;
	uint32_t margin;
	uint32_t width;
	uint32_t height;
	int magic;
	struct fontx_header *fontx_header;
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

void write_psf1_header(int fd, struct font_info *font_info)
{
	uint32_t height = font_info->height + font_info->margin;

	struct psf1_header h = {
		.magic    = { PSF1_MAGIC0, PSF1_MAGIC1 },
		.mode     = 0,
		.charsize = height * CHARLINE_BYTES(font_info->width)
	};

	void *data = &h;
	ssize_t size = sizeof(struct psf1_header);

	if (!font_info->magic) {
		size_t offset = offsetof(struct psf1_header, mode) * 8;
		data += offset;
		size -= offset;
	}

	if (write(fd, data, size) != size) {
		fprintf(stderr, "ERROR: Unable to write psf1 header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

void write_psf2_header(int fd, struct font_info *font_info)
{
	uint32_t height = font_info->height + font_info->margin;

	struct psf2_header h = {
		.magic      = { PSF2_MAGIC0, PSF2_MAGIC1, PSF2_MAGIC2, PSF2_MAGIC3 },
		.version    = 0,
		.headersize = sizeof(struct psf2_header),
		.flags      = 0,
		.length     = PSF2_CODEBLOCKS,
		.charsize   = height * CHARLINE_BYTES(font_info->width),
		.height     = height,
		.width      = font_info->width
	};

	void *data = &h;
	ssize_t size = h.headersize;

	if (!font_info->magic) {
		size_t offset = offsetof(struct psf2_header, version) * 8;
		data += offset;
		size -= offset;
	}

	if (write(fd, data, size) != size) {
		fprintf(stderr, "ERROR: Unable to write psf2 header: %s\n",
			errno ? strerror(errno) : "(Unknown)");
		exit(EXIT_FAILURE);
	}
}

void fix24(char *buf, uint32_t rsize, uint32_t wheight)
{
	uint32_t i = rsize;
	uint32_t j = wheight;

	while (i > 0) {
		buf[--j]  = (buf[--i] << 4) & 0xF0;
		buf[--j]  = (buf[  i] >> 4) & 0x0F;
		buf[  j] |= (buf[--i] << 4) & 0xF0;
		buf[--j]  =  buf[  i]       & 0xF0;
		buf[--j]  =  buf[--i];
	}
}

int copy_data(int in, int out, struct font_info *font_info)
{
	char buf[BUFSIZ];
	memset(buf, 0, BUFSIZ);

	uint32_t cb = CHARLINE_BYTES(font_info->width);

	uint32_t rsize = (font_info->fontx_header != NULL)
		? cb * font_info->height
		: font_info->width * font_info->height / 8;

	uint32_t mt = cb * (font_info->margin / 2);
	uint32_t mb = cb * ((font_info->margin + 1) / 2);

	uint32_t wheight = cb * font_info->height;
	uint32_t wsize = mt + wheight + mb;

	for (;;) {
		ssize_t sz = read(in, &buf[mt], rsize);

		if (sz == rsize) {
			if (font_info->fontx_header == NULL && font_info->height == 24) {
				fix24(&buf[mt], rsize, wheight);
			}

			if (write(out, buf, wsize) != wsize) {
				fprintf(stderr, "ERROR: Unable to write font data: %s\n",
					errno ? strerror(errno) : "(Unknown)");
				exit(EXIT_FAILURE);
			}

		} else if (sz == 0) {
			return 0;

		} else {
			fprintf(stderr, "ERROR: Unable to read font data: %s\n",
				errno ? strerror(errno) : "(Unknown)");
			exit(EXIT_FAILURE);
		}
	}
}

void usage(FILE *file, char *name)
{
	fprintf(file, "Usage: %s [-p psfver] [-m margin] [-i height] [-N]\n", name);
	fprintf(file, "    -p psfver: %d or %d (default: %d)\n", PSF_VERSION_1, PSF_VERSION_2, PSF_VERSION_2);
	fprintf(file, "    -m margin: %d - %d (default: %d)\n", MARGIN_MIN, MARGIN_MAX, MARGIN_MIN);
	fprintf(file, "    -i height: the font height (16,19,24) for PC DOS fonts\n");
	fprintf(file, "    -N       : no magic in the header\n");
}

void parse(int argc, char *argv[], struct font_info *font_info)
{
	char *name = argv[0];

	int opt = 0;

	optarg = NULL;
	optind = 1;
	opterr = 1;
	optopt = 0;

	while ((opt = getopt(argc, argv, "p:m:i:Nh")) != -1) {
		switch (opt) {
		case 'p':
			int p = atoi(optarg);

			if (p == PSF_VERSION_1 || p == PSF_VERSION_2) {
				font_info->psfver = p;
				break;
			} else {
				fprintf(stderr, "%s: invalid parameter: -p [%s]\n", name, optarg);
				exit(EXIT_FAILURE);
			}
		case 'm':
			int m = atoi(optarg);

			if (MARGIN_MIN <= m && m <= MARGIN_MAX) {
				font_info->margin = m;
				break;
			} else {
				fprintf(stderr, "%s: invalid parameter: -m [%s]\n", name, optarg);
				exit(EXIT_FAILURE);
			}
		case 'i':
			int i = atoi(optarg);

			switch (i) {
			case 16:
			case 19:
				font_info->width = 8;
				break;
			case 24:
				font_info->width = 12;
				break;
			default:
				fprintf(stderr, "%s: invalid parameter: -i [%s]\n", name, optarg);
				exit(EXIT_FAILURE);
			}

			font_info->height = i;
			break;
		case 'N':
			font_info->magic = 0;
			break;
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
	struct font_info font_info = {
		.psfver = PSF_VERSION_2,
		.margin = MARGIN_MIN,
		.width  = 0,
		.height = 0,
		.magic  = 1,
		.fontx_header = NULL
	};

	struct fontx_header fontx_header = {};

	parse(argc, argv, &font_info);

	if (font_info.height == 0) {
		read_fontx_header(STDIN_FILENO, &fontx_header);
		font_info.width = fontx_header.width;
		font_info.height = fontx_header.height;
		font_info.fontx_header = &fontx_header;
	}

	if (font_info.psfver == PSF_VERSION_1) {
		write_psf1_header(STDOUT_FILENO, &font_info);
	} else {
		write_psf2_header(STDOUT_FILENO, &font_info);
	}

	copy_data(STDIN_FILENO, STDOUT_FILENO, &font_info);

	return 0;
}
