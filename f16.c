/**************************************
**
** F16.C : Code to read and write 16-bit fractal data sets.  Uses
** strictly Targa16 type 10 files (run-length encoded 16-bit RGB).
*/

/* Lee Daniel Crocker      CompuServe: 73407,2030   <== Preferred
** 1380 Jewett Ave.               BIX: lcrocker
** Pittsburg, CA  94565        Usenet: ...!ames!pacbell!sactoh0!siva!lee
**
** This code is hereby placed in the public domain.  You are free to
** use, modify, usurp, laugh at, destroy, or otherwise abuse it in any
** way you see fit.
**
** "If you steal from one author it's plagiarism; if you steal from
** many it's research."  --Wilson Mizner
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "port.h"
#include "targa_lc.h"

extern char rlebuf[258];    /* RLE-state variables */
static int state, count, bufp;

/**************************************
**
** Create Targa16 type 10 file at give hsize and vsize.  Optional
** comment block may be included by setting csize to a non-zero value
** and cp to point to a comment block of that many bytes.
*/

FILE *t16_create(char *fname, int hs, int vs, int csize, U8 *cp)
{
    char filename[64];
    U8 header[HEADERSIZE];
    FILE *fp;

    memset(header, 0, HEADERSIZE);
    header[O_COMMENTLEN] = csize;
    header[O_FILETYPE] = T_RLERGB;
    STORE16(header[O_HSIZE], hs);
    STORE16(header[O_VSIZE], vs);
    header[O_ESIZE] = 16;
    header[O_FLAGS] = M_ORIGIN | 1;

    strcpy(filename, fname);
    if (strchr(filename, '.') == NULL) strcat(filename, ".TGA");
    if ((fp = fopen(filename, WRITEMODE)) == NULL) return NULL;
    fwrite(header, HEADERSIZE, 1, fp);
    if (csize) fwrite(cp, csize, 1, fp);

    state = count = bufp = 0;
    return fp;
}

/**************************************
**
** Open previously saved Targa16 type 10 file filling in hs, vs, and
** csize with values in the header.  If *csize is not zero, the block
** pointed to by cp is filled with the comment block.  The caller
** _must_ allocate 256 bytes for this purpose before calling.
*/

FILE *t16_open(char *fname, int *hs, int *vs, int *csize, U8 *cp)
{
    char filename[64];
    U8 header[HEADERSIZE];
    FILE *fp;

    strcpy(filename, fname);
    if (strchr(filename, '.') == NULL) strcat(filename, ".TGA");
    if ((fp = fopen(filename, READMODE)) == NULL) return NULL;

    fread(header, HEADERSIZE, 1, fp);
    if ((header[O_FILETYPE] != T_RLERGB) || (header[O_ESIZE] != 16)) {
        fclose(fp);
        return NULL;
    }
    GET16(header[O_HSIZE], *hs);
    GET16(header[O_VSIZE], *vs);
    if (*csize = header[O_COMMENTLEN]) fread(cp, *csize, 1, fp);

    state = count = bufp = 0;
    return fp;
}

int t16_putline(FILE *fp, int hs, U16 *data)
{
    int i, v, lastv;

    for (i=0; i<hs; ++i) {
        v = data[i];

        if (count == 128) {
            if (state == 2) {
                putc(0xFF, fp);
                fwrite(rlebuf, 1, 2, fp);
            } else if (state == 3) {
                putc(0x7F, fp);
                fwrite(rlebuf, 128, 2, fp);
            }
            state = bufp = count = 0;
        }

        if (state == 0) {
            STORE16(rlebuf[0], v);
            bufp = 2;
            count = state = 1;
        } else if (state == 1) {
            if (v == lastv) {
                state = 2;
            } else {
                state = 3;
                STORE16(rlebuf[bufp], v);
                bufp += 2;
            }
            ++count;
        } else if (state == 2) {
            if (v == lastv) ++count;
            else {
                putc((0x80|(count-1)), fp);
                fwrite(rlebuf, 1, 2, fp);
                STORE16(rlebuf[0], v);
                bufp = 2;
                count = state = 1;
            }
        } else if (state == 3) {
            if (v == lastv) {
                putc((count-2), fp);
                fwrite(rlebuf, count-1, 2, fp);
                STORE16(rlebuf[0], v);
                bufp = count = state = 2;
            } else {
                STORE16(rlebuf[bufp], v);
                bufp += 2;
                ++count;
            }
        }
        lastv = v;
    }
}

int t16_getline(FILE *fp, int hs, U16 *data)
{
    int i;

    for (i=0; i<hs; ++i) {
        if (state == 0) {
            bufp = 0;
            if ((count = getc(fp)) > 127) {
                state = 1;
                count -= 127;
                fread(rlebuf, 2, 1, fp);
            } else {
                state = 2;
                ++count;
                fread(rlebuf, 2, count, fp);
            }
        }
        GET16(rlebuf[bufp], data[i]);
        if (--count == 0) state = 0;
        if (state == 2) bufp += 2;
    }
}

int t16_flush(FILE *fp)
{
    if (state == 0) return(0);
    else if (state == 1) {
        putc(0, fp);
        fwrite(rlebuf, 2, 1, fp);
    } else if (state == 2) {
        putc((0x80|(count-1)), fp);
        fwrite(rlebuf, 2*count, 1, fp);
    } else {
        putc((count-1), fp);
        fwrite(rlebuf, 2*count, 1, fp);
    }
}

