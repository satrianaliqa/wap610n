/*
 * mkcsum.c - Add CRC to header and tailer of bootpImage. 
 * CRC is used for verifying image integrity during boot 
 * and when programming the image to flash.
 *
 * Copyright (C) 2008, 2009 Metalink Ltd.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


 * */

#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define HEADER_MAGIC 0xea000006

typedef struct {
  unsigned int magic;
  unsigned int pcksum2;
  unsigned int version;
  unsigned int version2;
  unsigned int filler[3];
  unsigned int cksum;
} header;

int main(int argc, char *argv[])
{
  header head;
  char buf[512*100];
  int fd;
  uLong crc = crc32(0L, Z_NULL, 0);
  int length;

  fd= open(argv[1], O_RDWR);
  if(read(fd, (void *)&head, (sizeof head)) != (sizeof head)) {
    fprintf(stderr,"Can't read header: %s\n", argv[1]);
    exit(1);
  }
  if (head.magic != HEADER_MAGIC) {
    fprintf(stderr,"Illegal magic in header - exiting crc calculation\n");
    exit(0);
  }
  printf("header cksum 0x%08.8x\n", head.cksum);
  length= head.pcksum2 - sizeof head;

  while (length > 0) {
    int len;
    len= (length > sizeof buf) ? sizeof buf : length;
    if(read(fd, buf, len) == len) {
      crc = crc32(crc, buf, len);
    } else {
      fprintf(stderr,"Can't read: %s\n", argv[1]);
      exit(1);
    }
    length -= len;
  }
  printf("csum 0x%08.8x\n", crc);
  head.cksum= crc;
  
  lseek(fd, 0, SEEK_SET);
  if(write(fd, (void *)&head, sizeof head) != sizeof head) {
    fprintf(stderr,"Can't write head\n");
    exit(1);
  }
  
  lseek(fd, head.pcksum2 + 12, SEEK_SET); // skip three ulong's
  if(write(fd, (void *)&crc, sizeof crc) != sizeof crc) {
    fprintf(stderr,"Can't write tail\n");
    exit(1);
  }
  exit(0);
}
