/*
   dejsonlz4 - Decompress Mozilla bookmark backup files
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
   (Mercurial) rev: c3f5e6079284 (2016-05-16) and carry their own license.
*/

/* Build: gcc -Wall -o dejsonlz4 dejsonlz4.c lz4.c */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#include "lz4.h"

const char mozlz4_magic[] = {109, 111, 122, 76, 122, 52, 48, 0};  /* "mozLz40\0" */
const int decomp_size = 4;  /* 4 bytes size come after the header */

void exit_usage(int code) {
    fprintf((code ? stderr : stdout), "%s",
"Usage: dejsonlz4 [-h] IN_FILE [OUT_FILE]\n"
"   -h: Display this help and exit.\n"
"Decompress Mozilla bookmark backup file IN_FILE to OUT_FILE.\n"
"If OUT_FILE is not provided or is '-' then decompress to standard output.\n"
           );
    exit(code);
}

void *file_to_mem(const char *fname, size_t *out_size)
{
    size_t fsize = 0;
    void *rv = 0;
    FILE *f = fopen(fname, "rb");
    if (!f)
        goto cleanup;
    if (fseek(f, 0, SEEK_END) < 0)
        goto cleanup;
    fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (!(rv = malloc(fsize)))
        goto cleanup;
    if (fsize != fread(rv, 1, fsize, f)) {
        free(rv);
        rv = 0;
    }

cleanup:
    if (f)
        fclose(f);
    if (rv && out_size)
        *out_size = fsize;
    return rv;
}

const size_t magic_size = sizeof mozlz4_magic;

int main(int argc, char **argv)
{
    size_t isize = 0, osize = 0;
    const char *iname = 0, *oname = 0;
    char *idata = 0, *odata = 0;
    FILE *ofile = 0;
    int rv = 1, dsize = -1, i;

    if ((argc > 3) || (argc < 2) || (argc > 1 && !strcmp(argv[1], "-h")))
        exit_usage(argc == 2 ? 0 : 1);
    iname = argv[1];
    if (argc > 2 && strcmp("-", argv[2]))
        oname = argv[2];

    if (!(idata = file_to_mem(iname, &isize))) {
        fprintf(stderr, "Error: cannot read file '%s'\n", iname);
        goto cleanup;
    }

    i = 0;
    while (i < isize && i < magic_size && idata[i] == mozlz4_magic[i])
        i++;
    if (i != magic_size || isize < magic_size + decomp_size) {
        fprintf(stderr, "Error: incorrect header or file too small\n");
        goto cleanup;
    }
    for (; i < magic_size + decomp_size; i++)
        osize += (unsigned char)idata[i] << (8 * (i - magic_size));

    if (!(odata = malloc(osize))) {
        fprintf(stderr, "Error: cannot allocate memory for output\n");
        goto cleanup;
    }
    if ((dsize = LZ4_decompress_safe(idata + i, odata, isize - i, osize)) < 0) {
        fprintf(stderr, "Error: decompression failed: %d\n", dsize);
        goto cleanup;
    }
    if (dsize != osize)
        fprintf(stderr, "Warning: decompressed file smaller than expected\n");

    if (oname) {
        if (!(ofile = fopen(oname, "wb"))) {
            fprintf(stderr, "Error: cannot open '%s' for writing\n", oname);
            goto cleanup;
        }
    } else {
#ifdef _WIN32
        /* Try changing stdout to binary mode, or else it messes with EOL chars.
         * Might not matter much for Mozilla files since they're text.
         */
        if (_setmode(_fileno(stdout), O_BINARY) == -1)
            fprintf(stderr, "Warning: cannot set output to binary mode\n");
#endif
        ofile = stdout;
    }

    if (dsize != fwrite(odata, 1, dsize, ofile)) {
        fprintf(stderr, "Error: cannot write to '%s'\n", oname ? oname : "<stdout>");
        goto cleanup;
    }

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
