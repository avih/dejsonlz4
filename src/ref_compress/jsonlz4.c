/*
   jsonlz4 - Compress with same format as Mozilla bookmarks backup files
   Copyright (C) 2016, Avi Halachmi
   BSD 2-Clause License (http://www.opensource.org/licenses/bsd-license.php)

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

       * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
       * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following disclaimer
   in the documentation and/or other materials provided with the
   distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   You can contact the author at :
   - Repository: https://github.com/avih/dejsonlz4
*/

/*
   lz4.c and lz4.h are verbatim copies from Mozilla source tree mfbt/lz4.*
   (Mercurial) rev: c3f5e6079284 (2016-05-12) and carry their own license.
*/

/* Build: copy src/ref_compress/jsonlz4.c to src/ and then:
 * gcc -Wall -o jsonlz4 jsonlz4.c lz4.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "lz4.h"

const char mozlz4_magic[] = {109, 111, 122, 76, 122, 52, 48, 0};  /* "mozLz40\0" */
const int decomp_size = 4;  /* 4 bytes size come after the header */

void exit_usage(int code) {
    fprintf((code ? stderr : stdout), "%s",
            "Usage: jsonlz4 [-h] IN_FILE OUT_FILE\n"
            "   -h  Display this help and exit.\n"
            "Compress IN_FILE to OUT_FILE with same format as Firefox bookmarks backup.\n"
            "If IN_FILE is '-', compress from standard input.\n"
            "If OUT_FILE is '-', compress to standard output.\n"
            "Note: IN_FILE is transferred to memory entirely before compressing.\n"
            "Compression is also done in memory entirely before output.\n"
            "Note: it's not recommended to use this tool, as it creates non standard files.\n"
           );
    exit(code);
}

/* If required, prevents EOL translations with f. Returns non-zero on failure */
int ensure_binary(FILE *f)
{
#ifdef _WIN32
    /* -1 is failure: https://msdn.microsoft.com/en-us/library/tw4k6df8.aspx */
    return _setmode(_fileno(f), O_BINARY) == -1;
#else
    return 0;  /* not required */
#endif
}

#define INITIAL_ALLOC_SIZE (32 * 1024)

/* if fname is NULL, reads from stdin till EOF */
void *file_to_mem(const char *fname, size_t *out_size)
{
    unsigned char *buf = 0, *rv = 0;
    size_t buf_size = 0, got = 0;
    FILE *f = fname ? fopen(fname, "rb") : stdin;
    if (!f)
        return 0;  /* can't do anything */
    if (!fname && ensure_binary(f))
        fprintf(stderr, "Warning: cannot set stdin to binary mode\n");

    do {
        buf_size = got ? got * 2 : INITIAL_ALLOC_SIZE;
        if (buf_size <= got || !(buf = realloc(buf, buf_size)))
            break;  /* size_t wrap-around or OOM: break before EOF */
        rv = buf;
        got += fread(buf + got, 1, buf_size - got, f);
    } while (got == buf_size);  /* otherwise feof(f) or ferror(f) */

    if (!buf || !feof(f) || ferror(f)) {
        if (rv)
            free(rv);
        rv = 0;
    }

    if (f && fname)
        fclose(f);
    if (rv && out_size)
        *out_size = got;
    return rv;
}

const size_t magic_size = sizeof mozlz4_magic;

#define ERR_CLEANUP(...) { fprintf(stderr, "Error: " __VA_ARGS__); goto cleanup; }

int main(int argc, char **argv)
{
    size_t isize = 0, osize = 0;
    const char *iname = 0, *oname = 0;
    char *idata = 0, *odata = 0;
    FILE *ofile = 0;
    int rv = 1, i, csize;

    /* process arguments */
    if (argc != 3)
        exit_usage(argc == 2 && !strcmp(argv[1], "-h") ? 0 : 1);
    if (strcmp("-", argv[1]))
        iname = argv[1];
    if (strcmp("-", argv[2]))
        oname = argv[2];

    /* read input file and allocate buffer for output file */
    if (!(idata = file_to_mem(iname, &isize)))
        ERR_CLEANUP("cannot read file '%s'\n", iname ? iname : "<stdin>");
    if (!(odata = malloc(magic_size + decomp_size + LZ4_compressBound(isize))))
        ERR_CLEANUP("cannot allocate memory for output\n");

    // write header and size of decompressed data at the beginning
    memcpy(odata, mozlz4_magic, magic_size);
    for (i = magic_size; i < magic_size + decomp_size; i++)
        odata[i] = (isize >> (8 * (i - magic_size))) & 0xff;

    /* compress */
    if (!(csize = LZ4_compress(idata, odata + i, isize)))
        ERR_CLEANUP("compression failed\n");
    osize = csize + magic_size + decomp_size;

    /* write output */
    if (!(ofile = oname ? fopen(oname, "wb") : stdout))
        ERR_CLEANUP("cannot open '%s' for writing\n", oname);
    if (!oname && ensure_binary(ofile))
        fprintf(stderr, "Warning: cannot set stdout to binary mode\n");
    if (osize != fwrite(odata, 1, osize, ofile))
        ERR_CLEANUP("cannot write to '%s'\n", oname ? oname : "<stdout>");

    rv = 0;

cleanup:
    if (ofile && oname)
        fclose(ofile);
    if (odata)
        free(odata);
    if (idata)
        free(idata);

    return rv;
}
