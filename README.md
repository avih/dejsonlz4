# dejsonlz4
Decompress Mozilla Firefox bookmarks backup files

Current Firefox bookmarks backup files are stored as non-standard file format
based on lz4 compression. These files have a `.jsonlz4` extension. Use
`dejsonlz4` to decompress them.

`lz4.c` and `lz4.h` at this repository are verbatim copies from the Mozilla
repository as of 2016-05-12 (as currently used by Firefox) [1].

## Usage:
```
Usage: dejsonlz4 [-h] IN_FILE [OUT_FILE]
   -h  Display this help and exit.
Decompress Mozilla bookmarks backup file IN_FILE to OUT_FILE.
If OUT_FILE is '-' or missing, decompress to standard output.
```

## Build:
- `gcc -Wall -o dejsonlz4 dejsonlz4.c lz4.c`

## Windows note:
- `dejsonlz4` on Windows does not support unicode path/file names at this time.

## References:
- Project page and source files: https://github.com/avih/dejsonlz4
- Releases and binary builds for Windows: https://github.com/avih/dejsonlz4/releases

### External resources:
- Mozilla Firefox
[bug 818587]( https://bugzilla.mozilla.org/show_bug.cgi?id=818587 ) - Compress
bookmark backups.
- Mozilla Firefox
[bug 1209390]( https://bugzilla.mozilla.org/show_bug.cgi?id=1209390 ) - Use
standard lz4 file format instead of the non-standard jsonlz4/mozlz4.

[1] Mozilla's mercurial repo rev. c3f5e6079284:
[lz4.h]( http://hg.mozilla.org/mozilla-central/file/c3f5e6079284/mfbt/lz4.h )
and [lz4.c]( http://hg.mozilla.org/mozilla-central/file/c3f5e6079284/mfbt/lz4.c )
