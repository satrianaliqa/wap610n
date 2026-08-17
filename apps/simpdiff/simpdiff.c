// Simple diff
// Open two files and binary compare them
//
// Copyright (c) 2008, 2009 Metalink Ltd.
// This program is released under the 
// GNU GPL ver 2.
// See LICENSE file for more details.

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void usage (char* prgName)
{
	printf("USAGE:\n%s [-s] <file1> <file2>\n  Options:\n  -s  Ignore file size differences (Check until end of smaller file).\n", prgName);
}

int main(int argc, char* argv[])
{
	char *filename1, *filename2;
	int file1, file2;
	int numRead1, numRead2;	
	unsigned int offset = 0;
	unsigned int ignoreSize = 0;
	unsigned int nameParam = 1;
	char buf1[2], buf2[2];
	
	if (argc < 3) {
		usage(argv[0]);
		return -1;
	}
	if (!strcmp(argv[1], "-s")) {
		ignoreSize = 1;
		nameParam++;
	}
	// argv 1 and 2 contain the filenames, unless -s was the first arg.
	filename1 = argv[nameParam++];
	filename2 = argv[nameParam];

	file1 = open(filename1, O_RDONLY);
	if (file1 < 0) {
		printf ("Unable to open %s\n", filename1);
		return -1;
	}

	file2 = open(filename2, O_RDONLY);
	if (file2 < 0) {
		printf ("Unable to open %s\n", filename2);
		return -1;
	}
	
	do {
		numRead1 = read(file1, buf1, 1);
		numRead2 = read(file2, buf2, 1);
		if (numRead1 > 0 && numRead2 > 0) {
			if (*buf1 != *buf2) {
				printf("Files are different. (%d vs. %d at offset %d)\n", *buf1, *buf2, offset);
				return 1;
			}
		}
		offset++;
	} while (numRead1 > 0 && numRead2 > 0);
	
	if (numRead1 > 0 || numRead2 > 0) {
		if (!ignoreSize) {
			printf("Files have different size.\n");
			return 2;
		}
	}
	return 0;
}
