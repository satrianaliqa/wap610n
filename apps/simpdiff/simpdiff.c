/* Simple diff
 * Open two files and binary compare them
 *
 * Copyright (c) 2008, 2009 Metalink Ltd.
 * This program is released under the 
 * GNU GPL ver 2.
 * See LICENSE file for more details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define CHUNK_SIZE 4096

static void usage(char *prgName)
{
	printf("USAGE:\n%s [-s] <file1> <file2>\n  Options:\n  -s  Ignore file size differences (Check until end of smaller file).\n", prgName);
}

static ssize_t read_chunk(int fd, unsigned char *buf, size_t count)
{
	size_t total;
	ssize_t n;

	total = 0;
	while (total < count) {
		n = read(fd, (char *)buf + total, count - total);
		if (n < 0) {
			return -1;
		}
		if (n == 0) {
			break;
		}
		total += (size_t)n;
	}
	return (ssize_t)total;
}

int main(int argc, char *argv[])
{
	char *filename1;
	char *filename2;
	int file1;
	int file2;
	ssize_t numRead1;
	ssize_t numRead2;
	size_t min_read;
	size_t i;
	unsigned int offset;
	unsigned int ignoreSize;
	unsigned int nameParam;
	unsigned char buf1[CHUNK_SIZE];
	unsigned char buf2[CHUNK_SIZE];

	offset = 0;
	ignoreSize = 0;
	nameParam = 1;

	if (argc < 3) {
		usage(argv[0]);
		return -1;
	}
	if (!strcmp(argv[1], "-s")) {
		ignoreSize = 1;
		nameParam++;
	}
	if (nameParam + 1 >= (unsigned int)argc) {
		usage(argv[0]);
		return -1;
	}
	/* argv 1 and 2 contain the filenames, unless -s was the first arg. */
	filename1 = argv[nameParam++];
	filename2 = argv[nameParam];

	file1 = open(filename1, O_RDONLY);
	if (file1 < 0) {
		printf("Unable to open %s\n", filename1);
		return -1;
	}

	file2 = open(filename2, O_RDONLY);
	if (file2 < 0) {
		printf("Unable to open %s\n", filename2);
		close(file1);
		return -1;
	}

	for (;;) {
		numRead1 = read_chunk(file1, buf1, CHUNK_SIZE);
		numRead2 = read_chunk(file2, buf2, CHUNK_SIZE);

		if (numRead1 < 0 || numRead2 < 0) {
			close(file1);
			close(file2);
			return -1;
		}

		min_read = (size_t)((numRead1 < numRead2) ? numRead1 : numRead2);
		for (i = 0; i < min_read; i++) {
			if (buf1[i] != buf2[i]) {
				printf("Files are different. (%d vs. %d at offset %d)\n",
				       (int)buf1[i], (int)buf2[i], (int)(offset + i));
				close(file1);
				close(file2);
				return 1;
			}
		}
		offset += (unsigned int)min_read;

		if (numRead1 != numRead2) {
			if (!ignoreSize) {
				printf("Files have different size.\n");
				close(file1);
				close(file2);
				return 2;
			}
			close(file1);
			close(file2);
			return 0;
		}

		if (numRead1 == 0) {
			break;
		}
	}

	close(file1);
	close(file2);
	return 0;
}
