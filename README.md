# dejsonlz4
Decompressor for Mozilla/Firefox bookmark backup files

Mozilla Firefox currently uses unofficial lz4 compression for bookmark backup files,
and `dejsonlz4` can currently be used to decompress them. These files have an extension `.jsonlz4`.

This repository includes verbatim copies of `lz4.c` and `lz4.h` from the Mozilla repository
as of 2016-05-16 (as currently used by Firefox).

## Usage:
```
Usage: dejsonlz4 [-h] IN_FILE [OUT_FILE]
   -h: Display this help and exit.
Decompress Mozilla bookmark backup file IN_FILE to OUT_FILE.
If OUT_FILE is not provided or is '-' then decompress to standard output.
```

## Build:
- `gcc -Wall -o dejsonlz4 dejsonlz4.c lz4.c`

## Windows note:
- `dejsonlz4` currently does not support unicode file names, for the sake of simpler code.
