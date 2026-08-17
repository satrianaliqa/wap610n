/* 
  LzmaDecode.h
  LZMA Decoder interface

  LZMA SDK 4.16 Copyright (c) 1999-2005 Igor Pavlov (2005-03-18)
  http://www.7-zip.org/

  LZMA SDK is licensed under two licenses:
  1) GNU Lesser General Public License (GNU LGPL)
  2) Common Public License (CPL)
  It means that you can select one of these two licenses and 
  follow rules of that license.

  SPECIAL EXCEPTION:
  Igor Pavlov, as the author of this code, expressly permits you to 
  statically or dynamically link your code (or bind by name) to the 
  interfaces of this file without subjecting your linked code to the 
  terms of the CPL or GNU LGPL. Any modifications or additions 
  to this file, however, are subject to the LGPL or CPL terms.
*/

#ifndef __LZMADECODE_H
#define __LZMADECODE_H

/* #define _LZMA_IN_CB */
/* Use callback for input data */

/* #define _LZMA_OUT_READ */
/* Use read function for output data */

#define _LZMA_PROB32
/* It can increase speed on some 32-bit CPUs, 
   but memory usage will be doubled in that case */

#define _LZMA_LOC_OPT 
/* Enable local speed optimizations inside code */

#ifdef _LZMA_PROB32
#define CProb u32
#else
#define CProb unsigned short
#endif

#define LZMA_RESULT_OK 0
#define LZMA_RESULT_DATA_ERROR 1
#define LZMA_RESULT_NOT_ENOUGH_MEM 2

#ifdef _LZMA_IN_CB
typedef struct _ILzmaInCallback
{
	int (*Read)(void *object, unsigned char **buffer, u32 *bufferSize);
} ILzmaInCallback;
#endif

#define LZMA_BASE_SIZE 1846
#define LZMA_LIT_SIZE 768

/* 
bufferSize = (LZMA_BASE_SIZE + (LZMA_LIT_SIZE << (lc + lp)))* sizeof(CProb)
bufferSize += 100 in case of _LZMA_OUT_READ
by default CProb is unsigned short, 
but if specify _LZMA_PROB_32, CProb will be u32(unsigned int)
*/

#ifdef _LZMA_OUT_READ
int LzmaDecoderInit(
	unsigned char *buffer, u32 bufferSize,
	int lc, int lp, int pb,
	unsigned char *dictionary, u32 dictionarySize,
#ifdef _LZMA_IN_CB
	ILzmaInCallback *inCallback
#else
	unsigned char *inStream, u32 inSize
#endif /* _LZMA_IN_CB */
);
#endif /* _LZMA_OUT_READ */

int LzmaDecode(
	unsigned char *buffer, 
#ifndef _LZMA_OUT_READ
	u32 bufferSize,
	int lc, int lp, int pb,
#ifdef _LZMA_IN_CB
	ILzmaInCallback *inCallback,
#else
	unsigned char *inStream, u32 inSize,
#endif /*  _LZMA_IN_CB */
#endif /* _LZMA_OUT_READ */
	unsigned char *outStream, u32 outSize,
	u32 *outSizeProcessed);
#endif /* __LZMADECODE_H */
