/* CRC.h: A header file that was added for the CRC.c file.
   This is not part of the original code that was taken from 
   The Regents of the University of California.
*/

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

EXTERN_C int crc(int fd, unsigned long *cval, unsigned long *clen);
