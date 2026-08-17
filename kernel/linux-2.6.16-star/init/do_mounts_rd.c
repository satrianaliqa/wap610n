
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/minix_fs.h>
#include <linux/ext2_fs.h>
#include <linux/romfs_fs.h>
#include <linux/cramfs_fs.h>
#include <linux/initrd.h>
#include <linux/string.h>

#include "do_mounts.h"

int __initdata rd_prompt = 1;/* 1 = prompt for RAM disk, 0 = don't prompt */

static int __init prompt_ramdisk(char *str)
{
	rd_prompt = simple_strtol(str,NULL,0) & 1;
	return 1;
}
__setup("prompt_ramdisk=", prompt_ramdisk);

int __initdata rd_image_start;		/* starting block # of image */

static int __init ramdisk_start_setup(char *str)
{
	rd_image_start = simple_strtol(str,NULL,0);
	return 1;
}
__setup("ramdisk_start=", ramdisk_start_setup);


#ifdef CONFIG_BLK_DEV_RAM_GZ
static int __init gz_load(int in_fd, int out_fd);
#endif /* CONFIG_BLK_DEV_RAM_GZ */

#ifdef CONFIG_BLK_DEV_RAM_LZMA
static int __init lzma_load(int in_fd, int out_fd);
#endif /* CONFIG_BLK_DEV_RAM_LZMA */

/*
 * This routine tries to find a RAM disk image to load, and returns the
 * number of blocks to read for a non-compressed image, a negative value
 * if the image is a compressed image, and RD_ERROR if an image with
 * the right magic numbers (see below) could not be found.
 *
 * We currently check for the following magic numbers:
 * 	minix
 * 	ext2
 *	romfs
 *	cramfs
 * 	compressed image formats (gzip, lzma)
 */
static int __init 
identify_ramdisk_image(int fd, int start_block)
{
	const int size = 512;
	struct minix_super_block *minixsb;
	struct ext2_super_block *ext2sb;
	struct romfs_super_block *romfsb;
	struct cramfs_super *cramfsb;
	int nblocks = RD_ERROR;
	unsigned char *buf;

	buf = kmalloc(size, GFP_KERNEL);
	if (buf == 0)
		return RD_ERROR;

	minixsb = (struct minix_super_block *) buf;
	ext2sb = (struct ext2_super_block *) buf;
	romfsb = (struct romfs_super_block *) buf;
	cramfsb = (struct cramfs_super *) buf;
	memset(buf, 0xe5, size);

	/*
	 * Read block 0 to test for gzipped kernel
	 */
	sys_lseek(fd, start_block * BLOCK_SIZE, 0);
	sys_read(fd, buf, size);

	/*
	 * If it matches the gzip magic numbers, return CRAMDISK_GZ
	 */
	if (buf[0] == 037 && ((buf[1] == 0213) || (buf[1] == 0236))) {
		printk(KERN_NOTICE
		       "RAMDISK: Gzip compressed image found at block %d\n",
		       start_block);
		nblocks = CRAMDISK_GZ;
		goto done;
	}

	/*
	 * Unfortunally lzma has no magic number, return CRAMDISK_LZMA
	 * but byte 9 to 12 has to be zero
	 */
	if (buf[9] == 0 && buf[10] == 0 && buf[11] == 0 && buf[12] == 0) {
		printk(KERN_NOTICE
			"RAMDISK: lzma compressed image found at block %d\n",
			start_block);
		nblocks = CRAMDISK_LZMA;
		goto done;
	}
	
	/* romfs is at block zero too */
	if (romfsb->word0 == ROMSB_WORD0 &&
	    romfsb->word1 == ROMSB_WORD1) {
		printk(KERN_NOTICE
		       "RAMDISK: romfs filesystem found at block %d\n",
		       start_block);
		nblocks = (ntohl(romfsb->size)+BLOCK_SIZE-1)>>BLOCK_SIZE_BITS;
		goto done;
	}

	if (cramfsb->magic == CRAMFS_MAGIC) {
		printk(KERN_NOTICE
		       "RAMDISK: cramfs filesystem found at block %d\n",
		       start_block);
		nblocks = (cramfsb->size + BLOCK_SIZE - 1) >> BLOCK_SIZE_BITS;
		goto done;
	}

	/*
	 * Read block 1 to test for minix and ext2 superblock
	 */
	sys_lseek(fd, (start_block+1) * BLOCK_SIZE, 0);
	sys_read(fd, buf, size);

	/* Try minix */
	if (minixsb->s_magic == MINIX_SUPER_MAGIC ||
	    minixsb->s_magic == MINIX_SUPER_MAGIC2) {
		printk(KERN_NOTICE
		       "RAMDISK: Minix filesystem found at block %d\n",
		       start_block);
		nblocks = minixsb->s_nzones << minixsb->s_log_zone_size;
		goto done;
	}

	/* Try ext2 */
	if (ext2sb->s_magic == cpu_to_le16(EXT2_SUPER_MAGIC)) {
		printk(KERN_NOTICE
		       "RAMDISK: ext2 filesystem found at block %d\n",
		       start_block);
		nblocks = le32_to_cpu(ext2sb->s_blocks_count) <<
			le32_to_cpu(ext2sb->s_log_block_size);
		goto done;
	}

	printk(KERN_NOTICE
	       "RAMDISK: Couldn't find valid RAM disk image starting at %d.\n",
	       start_block);
	
done:
	sys_lseek(fd, start_block * BLOCK_SIZE, 0);
	kfree(buf);
	return nblocks;
}

int __init rd_load_image(char *from)
{
	int res = 0;
	int in_fd, out_fd;
	unsigned long rd_blocks, devblocks;
	int nblocks, i, disk;
	char *buf = NULL;
	unsigned short rotate = 0;
#if !defined(CONFIG_S390) && !defined(CONFIG_PPC_ISERIES)
	char rotator[4] = { '|' , '/' , '-' , '\\' };
#endif

	out_fd = sys_open("/dev/ram", O_RDWR, 0);
	if (out_fd < 0)
		goto out;

	in_fd = sys_open(from, O_RDONLY, 0);
	if (in_fd < 0)
		goto noclose_input;

	nblocks = identify_ramdisk_image(in_fd, rd_image_start);
	switch (nblocks) {
		case CRAMDISK_LZMA :     /* lzma image found */
#ifdef CONFIG_BLK_DEV_RAM_LZMA
			if (lzma_load(in_fd, out_fd) == 0)
				goto successful_load;
#else
			printk(KERN_ALERT "RAMDISK: you don't have "
					"CONFIG_BLK_DEV_RAM_LZMA\n");
#endif
			break;
		case CRAMDISK_GZ :      /* gzip image found */
#ifdef CONFIG_BLK_DEV_RAM_GZ
			if (gz_load(in_fd, out_fd) == 0)
				goto successful_load;
#else
			printk(KERN_ALERT "RAMDISK: you don't have "
					"CONFIG_BLK_DEV_RAM_GZ\n");
#endif
			break;
		default :
			break;
	}

	/*
	 * NOTE NOTE: nblocks is not actually blocks but
	 * the number of kibibytes of data to load into a ramdisk.
	 * So any ramdisk block size that is a multiple of 1KiB should
	 * work when the appropriate ramdisk_blocksize is specified
	 * on the command line.
	 *
	 * The default ramdisk_blocksize is 1KiB and it is generally
	 * silly to use anything else, so make sure to use 1KiB
	 * blocksize while generating ext2fs ramdisk-images.
	 */
	if (sys_ioctl(out_fd, BLKGETSIZE, (unsigned long)&rd_blocks) < 0)
		rd_blocks = 0;
	else
		rd_blocks >>= 1;

	if (nblocks > rd_blocks) {
		printk("RAMDISK: image too big! (%dKiB/%ldKiB)\n",
		       nblocks, rd_blocks);
		goto done;
	}
		
	/*
	 * OK, time to copy in the data
	 */
	if (sys_ioctl(in_fd, BLKGETSIZE, (unsigned long)&devblocks) < 0)
		devblocks = 0;
	else
		devblocks >>= 1;

	if (strcmp(from, "/initrd.image") == 0)
		devblocks = nblocks;

	if (devblocks == 0) {
		printk(KERN_ERR "RAMDISK: could not determine device size\n");
		goto done;
	}

	buf = kmalloc(BLOCK_SIZE, GFP_KERNEL);
	if (buf == 0) {
		printk(KERN_ERR "RAMDISK: could not allocate buffer\n");
		goto done;
	}

	printk(KERN_NOTICE "RAMDISK: Loading %dKiB [%ld disk%s] into"
		"ram disk... ",	nblocks, ((nblocks-1)/devblocks)+1,
		nblocks>devblocks ? "s" : "");
	for (i = 0, disk = 1; i < nblocks; i++) {
		if (i && (i % devblocks == 0)) {
			printk("done disk #%d.\n", disk++);
			rotate = 0;
			if (sys_close(in_fd)) {
				printk("Error closing the disk.\n");
				goto noclose_input;
			}
			change_floppy("disk #%d", disk);
			in_fd = sys_open(from, O_RDONLY, 0);
			if (in_fd < 0)  {
				printk("Error opening disk.\n");
				goto noclose_input;
			}
			printk("Loading disk #%d... ", disk);
		}
		sys_read(in_fd, buf, BLOCK_SIZE);
		sys_write(out_fd, buf, BLOCK_SIZE);
#if !defined(CONFIG_S390) && !defined(CONFIG_PPC_ISERIES)
		if (!(i % 16)) {
			printk("%c\b", rotator[rotate & 0x3]);
			rotate++;
		}
#endif
	}
	printk("done.\n");

successful_load:
	res = 1;
done:
	sys_close(in_fd);
noclose_input:
	sys_close(out_fd);
out:
	kfree(buf);
	return res;
}

int __init rd_load_disk(int n)
{
	if (rd_prompt)
		change_floppy("root floppy disk to be loaded into RAM disk");
	create_dev("/dev/root", ROOT_DEV, root_device_name);
	create_dev("/dev/ram", MKDEV(RAMDISK_MAJOR, n), NULL);
	return rd_load_image("/dev/root");
}

#ifdef CONFIG_BLK_DEV_RAM_GZ

/*
 * gzip declarations
 */

#define OF(args)  args

#ifndef memzero
#define memzero(s, n)     memset ((s), 0, (n))
#endif

typedef unsigned char  uch;
typedef unsigned short ush;
typedef unsigned long  ulg;

#define INBUFSIZ 4096
#define WSIZE 0x8000    /* window size--must be a power of two, and */
			/*  at least 32K for zip's deflate method */

static uch *inbuf;
static uch *window;

static unsigned insize;  /* valid bytes in inbuf */
static unsigned inptr;   /* index of next byte to be processed in inbuf */
static unsigned outcnt;  /* bytes in output buffer */
static int exit_code;
static int unzip_error;
static long bytes_out;
static int crd_infd, crd_outfd;

#define get_byte()  (inptr < insize ? inbuf[inptr++] : fill_inbuf())
		
/* Diagnostic functions (stubbed out) */
#define Assert(cond,msg)
#define Trace(x)
#define Tracev(x)
#define Tracevv(x)
#define Tracec(c,x)
#define Tracecv(c,x)

#define STATIC static
#define INIT __init

static int  __init fill_inbuf(void);
static void __init flush_window(void);
static void __init *malloc(size_t size);
static void __init free(void *where);
static void __init error(char *m);
static void __init gzip_mark(void **);
static void __init gzip_release(void **);

#include "../lib/inflate.c"

static void __init *malloc(size_t size)
{
	return kmalloc(size, GFP_KERNEL);
}

static void __init free(void *where)
{
	kfree(where);
}

static void __init gzip_mark(void **ptr)
{
}

static void __init gzip_release(void **ptr)
{
}


/* ===========================================================================
 * Fill the input buffer. This is called only when the buffer is empty
 * and at least one byte is really needed.
 * Returning -1 does not guarantee that gunzip() will ever return.
 */
static int __init fill_inbuf(void)
{
	if (exit_code) return -1;
	
	insize = sys_read(crd_infd, inbuf, INBUFSIZ);
	if (insize == 0) {
		error("RAMDISK: ran out of compressed data");
		return -1;
	}

	inptr = 1;

	return inbuf[0];
}

/* ===========================================================================
 * Write the output window window[0..outcnt-1] and update crc and bytes_out.
 * (Used for the decompressed data only.)
 */
static void __init flush_window(void)
{
    ulg c = crc;         /* temporary variable */
    unsigned n, written;
    uch *in, ch;
    
    written = sys_write(crd_outfd, window, outcnt);
    if (written != outcnt && unzip_error == 0) {
	printk(KERN_ERR "RAMDISK: incomplete write (%d != %d) %ld\n",
	       written, outcnt, bytes_out);
	unzip_error = 1;
    }
    in = window;
    for (n = 0; n < outcnt; n++) {
	    ch = *in++;
	    c = crc_32_tab[((int)c ^ ch) & 0xff] ^ (c >> 8);
    }
    crc = c;
    bytes_out += (ulg)outcnt;
    outcnt = 0;
}

static void __init error(char *x)
{
	printk(KERN_ERR "%s\n", x);
	exit_code = 1;
	unzip_error = 1;
}

static int __init gz_load(int in_fd, int out_fd)
{
	int result;

	insize = 0;	/* valid bytes in inbuf */
	inptr = 0;	/* index of next byte to be processed in inbuf */
	outcnt = 0;	/* bytes in output buffer */
	exit_code = 0;
	bytes_out = 0;
	crc = (ulg)0xffffffffL; /* shift register contents */

	crd_infd = in_fd;
	crd_outfd = out_fd;
	inbuf = kmalloc(INBUFSIZ, GFP_KERNEL);
	if (inbuf == 0) {
		printk(KERN_ERR "RAMDISK: Couldn't allocate gzip buffer\n");
		return -1;
	}
	window = kmalloc(WSIZE, GFP_KERNEL);
	if (window == 0) {
		printk(KERN_ERR "RAMDISK: Couldn't allocate gzip window\n");
		kfree(inbuf);
		return -1;
	}
	makecrc();
	result = gunzip();
	if (unzip_error)
		result = 1;
	kfree(inbuf);
	kfree(window);
	return result;
}

#endif  /* ONFIG_BLK_DEV_RAM_GZ */

#ifdef CONFIG_BLK_DEV_RAM_LZMA
#include <linux/vmalloc.h>
/* the lzma decompression function will use a callback function to get
 * it's input data */
#define _LZMA_IN_CB
/* the lzma decompression function will only write out small pieces instead
 * of everything to a single address, the disadvantage of this is that we
 * need extra memory for the dictionary, the default size is 8 MB */
#define _LZMA_OUT_READ 
#define _LZMA_READ_COMPRESSED_BUFFER_SIZE 0x10000
#define _LZMA_WRITE_BUFFER_SIZE 0x10000

#include <../lib/lzmadecode.h>
#include <../lib/lzmadecode.c>

typedef struct _cbuffer
{
	ILzmaInCallback in_callback;
	unsigned char *buffer;
	int lzma_read_fd;
} cbuffer; 
  
static int lzma_read_in(void *obj, unsigned char **buffer, unsigned int *size)
{
	cbuffer *bo = obj;
	int read_size;
	/* try to read _LZMA_READ_COMPRESSED_BUFFER_SIZE bytes */
	read_size = sys_read(bo->lzma_read_fd, bo->buffer,
		_LZMA_READ_COMPRESSED_BUFFER_SIZE);
	*size = read_size;
	*buffer = bo->buffer;
	return LZMA_RESULT_OK;
}

static int __init lzma_load(int in_fd, int out_fd)
{
	unsigned char properties[5], prop0;
	unsigned char *dictionary, *out_buffer = 0;
	unsigned int out_size, lzma_internal_size, size_processed;
	unsigned int now_pos, dictionary_size  = 0;
	void *lzma_internal;
	int lc, lp, pb;
	int i, res, return_val;
	cbuffer bo;
	
	if (sys_read(in_fd, (unsigned char *)&properties, 5) == 0) {
		printk(KERN_ERR "RAMDISK: ran out of compressed data\n");
		return -1;
	}

	out_size = 0;
	for (i = 0; i < 4; i++) {
		unsigned char b;
		if (sys_read(in_fd, &b, 1) == 0) {
			printk(KERN_ERR "RAMDISK: ran out of compressed "
				"data\n");
			return -1;
		}
		out_size += (unsigned int)(b) << (i * 8);
	}
	
	for (i = 0; i < 4; i++) {
		unsigned char b;
		if (sys_read(in_fd, &b, 1) == 0) {
			printk(KERN_ERR "RAMDISK: ran out of compressed "
				"data\n");
			return -1;
		}
		if (b!=0) {
			printk(KERN_ERR "RAMDISK: either this is not a lzma"
				"compressed ramdisk or it's bigger 4 GB\n");
			return -1;
		}
	}
	
	prop0 = properties[0];
	if (prop0 >= (9*5*5)) { 
		printk(KERN_ERR "RAMDISK: lzma Properties error\n");
		return -1;
	}

	pb = prop0 / 45;
	prop0 = prop0 % 45;
	lp = prop0 / 9;
	lc = prop0 % 9;

	lzma_internal_size = (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp)))
		* sizeof(CProb);
	lzma_internal_size += 100; /* because we are using _LZMA_OUT_READ */
	
	lzma_internal = vmalloc(lzma_internal_size);
	if (lzma_internal == 0) { 
		printk(KERN_ERR "RAMDISK: failed to get memory "
			"for lzma_internal\n");
		return -1;
	}

	bo.in_callback.Read = lzma_read_in;
	bo.lzma_read_fd = in_fd;
	bo.buffer = vmalloc(_LZMA_READ_COMPRESSED_BUFFER_SIZE);
	if (bo.buffer == 0) { 
		printk(KERN_ERR "RAMDISK: failed to get memory "
			"for bo.buffer\n");
		return_val = -1;
		goto free_lzma_internal;
	}

	for (i = 0; i < 4; i++) {
		dictionary_size += (u32)(properties[1 + i]) << (i * 8);
	}
	if (dictionary_size == 0) {
		/* LZMA decoder can not work with dictionary_size = 0 */
		dictionary_size = 1;
	}
	dictionary = vmalloc(dictionary_size);
	if (dictionary == 0) { 
		printk(KERN_ERR "RAMDISK: failed to get memory "
			"for dictionary\n");
		return_val = -1;
		goto free_bo_buffer;
	}
	res = LzmaDecoderInit((unsigned char *)lzma_internal,
		lzma_internal_size,
		lc, lp, pb,
		dictionary, dictionary_size,
		&bo.in_callback);
	if (res == 0) {
		out_buffer = vmalloc(_LZMA_WRITE_BUFFER_SIZE);
		if (out_buffer == 0) { 
			printk(KERN_ERR "RAMDISK: failed to get memory"
				"for out_buffer\n");
			return_val = -1;
			goto free_dictionary;
		}
		for (now_pos = 0; now_pos < out_size;) {
			res = LzmaDecode((unsigned char *)lzma_internal,
				out_buffer, _LZMA_WRITE_BUFFER_SIZE,
				&size_processed);
			if (res != 0)
				break;
			if (size_processed == 0) {
				out_size = now_pos;
				break;
			}
			now_pos += size_processed;
			res = sys_write(out_fd, out_buffer, size_processed);
			if (res != size_processed) {
				printk(KERN_ERR "can't write everything,"
					"the ramdisk is too small\n");
				return_val = -1;
				goto free_out_buffer;
			}
		}
	}
	return_val = 0;

free_out_buffer:
	vfree(out_buffer);
free_dictionary:
	vfree(dictionary);
free_bo_buffer:
	vfree(bo.buffer);
free_lzma_internal:
	vfree(lzma_internal);
	
	return return_val;
}
#endif /* CONFIG_BLK_DEV_RAM_LZMA */
