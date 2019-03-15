/*
 * Copyright (c) 2016-present, Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */


#include <stdlib.h>    // malloc, free, exit
#include <stdio.h>     // fprintf, perror, feof, fopen, etc.
#include <string.h>    // strlen, memset, strcat
#include <zstd.h>      // presumes zstd library is installed
#include "utils.h"

static void compressFile_orDie(const char* fname, const char* outName, int cLevel)
{
    FILE* const fin  = fopen_orDie(fname, "rb");
    FILE* const fout = fopen_orDie(outName, "wb");
    size_t const buffInSize = ZSTD_CStreamInSize();    /* can always read one full block */
    void*  const buffIn  = malloc_orDie(buffInSize);
    size_t const buffOutSize = ZSTD_CStreamOutSize();  /* can always flush a full block */
    void*  const buffOut = malloc_orDie(buffOutSize);

    ZSTD_CCtx* const cstream = ZSTD_createCStream();
    if (cstream==NULL) { fprintf(stderr, "ZSTD_createCStream() error \n"); exit(10); }
    // size_t const initResult = ZSTD_initCStream(cstream, cLevel);
    ZSTD_cParameter param = ZSTD_c_compressionLevel;
    size_t const initResult = ZSTD_CCtx_setParameter(cstream,param,cLevel);

    if (ZSTD_isError(initResult)) {
        fprintf(stderr, "ZZSTD_CCtx_setParameter() error : %s \n",
                    ZSTD_getErrorName(initResult));
        exit(11);
    }

    size_t read, toRead = buffInSize;
    ZSTD_EndDirective endDir = ZSTD_e_continue;

    while( (read = fread_orDie(buffIn, toRead, fin)) ) {
        ZSTD_inBuffer input = { buffIn, read, 0 };

        if(read < toRead){
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            ZSTD_EndDirective = ZSTD_e_end;
            toRead = ZSTD_compressStream2(cstream, &output , &input, endDir);

        if (ZSTD_isError(toRead)) {
                    fprintf(stderr, "ZSTD_compressStream() error : %s \n",
                                    ZSTD_getErrorName(toRead));
                    exit(12);
                } 
        if (toRead) { fprintf(stderr, "not fully flushed"); exit(13); }
        }
        else{
            while (input.pos < input.size) {
                ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
                toRead = ZSTD_compressStream2(cstream, &output , &input, endDir);   /* toRead is guaranteed to be <= ZSTD_CStreamInSize() */
               if (ZSTD_isError(toRead)) {
                    fprintf(stderr, "ZSTD_compressStream() error : %s \n",
                                    ZSTD_getErrorName(toRead));
                    exit(12);
                }   
            }
          
            if (toRead > buffInSize) toRead = buffInSize;   /* Safely handle case when `buffInSize` is manually changed to a value < ZSTD_CStreamInSize()*/
             fwrite_orDie(buffOut, output.pos, fout);
        }
    }

    ZSTD_freeCStream(cstream);
    fclose_orDie(fout);
    fclose_orDie(fin);    free(buffIn);
    free(buffOut);
}


static char* createOutFilename_orDie(const char* filename)
{
    size_t const inL = strlen(filename);
    size_t const outL = inL + 5;
    void* const outSpace = malloc_orDie(outL);
    memset(outSpace, 0, outL);
    strcat(outSpace, filename);
    strcat(outSpace, ".zst");
    return (char*)outSpace;
}

int main(int argc, const char** argv)
{
    const char* const exeName = argv[0];

    if (argc!=2) {
        printf("wrong arguments\n");
        printf("usage:\n");
        printf("%s FILE\n", exeName);
        return 1;
    }

    const char* const inFilename = argv[1];

    char* const outFilename = createOutFilename_orDie(inFilename);
    compressFile_orDie(inFilename, outFilename, 1);

    free(outFilename);   /* not strictly required, since program execution stops there,
                          * but some static analyzer main complain otherwise */
    return 0;
}
