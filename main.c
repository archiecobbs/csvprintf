
//
// csvprintf - Simple CSV file parser for the UNIX command line
// 
// Copyright 2010 Archie L. Cobbs <archie@dellroad.org>
// 
// Licensed under the Apache License, Version 2.0 (the "License"); you may
// not use this file except in compliance with the License. You may obtain
// a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// 

#include "csvprintf.h"

#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <iconv.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_QUOTE_CHAR      '"'
#define DEFAULT_FSEP_CHAR       ','
#define XML_OUTPUT_ENCODING     "UTF-8"

struct col {
    char    *buf;
    size_t  len;
    size_t  alloc;
};

struct row {
    char    **fields;
    size_t  num;
    size_t  alloc;
};

static int quote = DEFAULT_QUOTE_CHAR;
static int fsep = DEFAULT_FSEP_CHAR;

static int parsechar(const char *str);
static int parsefmt(char *fmt, const struct row *column_names, unsigned int **argsp);
static int readcol(FILE *fp, struct row *row, int *linenum);
static int readqcol(FILE *fp, struct col *col, int *linenum);
static int readuqcol(FILE *fp, struct col *col, int *linenum);
static int readch(FILE *fp, int collapse);
static void freerow(struct row *row);
static void print_xml_tag_name(const char *tag, int linenum);
static int decode_utf8(const char *const obuf, size_t olen, int *lenp, int linenum);
static void convert_to_utf8(iconv_t icd, struct row *row, int linenum);
static const char *escape_xml_char(int uchar);
static char *eatwidthprec(const char *fspec, const char *desc, const struct row *column_names,
    char *s, int *nargs, unsigned int *args);
static char *eataccessor(const char *fspec, const char *desc, const struct row *column_names,
    char *s, int *nargs, unsigned int *args);
static void addcolumn(struct row *row, const struct col *col);
static void addchar(struct col *col, int ch);
static void trim(struct col *col);
static void usage(void);
static void version(void);

int
main(int argc, char **argv)
{
    const char *input = "-";
    const char *encoding = "ISO-8859-1";
    char *format = NULL;
    iconv_t icd = NULL;
    FILE *fp = NULL;
    struct row row;
    struct row column_names;
    unsigned int *args = NULL;
    int read_column_names = 0;
    int first_row = 0;
    int nargs = 0;
    int file_done;
    int linenum;
    int xml = 0;        // 1 = normal, 2 = use first row column names for XML tags
    int ch;

    // Initialize
    memset(&row, 0, sizeof(row));
    memset(&column_names, 0, sizeof(column_names));

    // Parse command line
    while ((ch = getopt(argc, argv, "e:f:hiq:s:vxX")) != -1) {
        switch (ch) {
        case 'e':
            encoding = optarg;
            break;
        case 'f':
            input = optarg;
            break;
        case 'i':
            read_column_names = 1;
            break;
        case 'x':
            xml = 1;
            break;
        case 'X':
            read_column_names = 1;
            xml = 2;
            break;
        case 'q':
            if ((quote = parsechar(optarg)) == -1)
                errx(1, "invalid argument to \"-%c\"", ch);
            break;
        case 's':
            if ((fsep = parsechar(optarg)) == -1)
                errx(1, "invalid argument to \"-%c\"", ch);
            break;
        case 'h':
            usage();
            exit(0);
        case 'v':
            version();
            exit(0);
        case '?':
        default:
            usage();
            exit(1);
        }
    }
    argc -= optind;
    argv += optind;
    if (argc != (xml ? 0 : 1)) {
        usage();
        exit(1);
    }

    // Sanity check
    if (quote == fsep)
        err(1, "quote and field separators cannot be the same character");

    // Get and (maybe) parse format string (non-XML only)
    if (!xml) {
        format = argv[0];

        // Parse format string - unless we need to defer
        if (!read_column_names)
            nargs = parsefmt(format, NULL, &args);
    }

    // Open input
    if (strcmp(input, "-") == 0)
        fp = stdin;
    else if ((fp = fopen(input, "r")) == NULL)
        err(1, "%s", input);

    // XML opening
    if (xml) {
        if ((icd = iconv_open(XML_OUTPUT_ENCODING, encoding)) == (iconv_t)-1)
            err(1, "%s", encoding);
        printf("<?xml version=\"1.0\" encoding=\"%s\"?>\n", XML_OUTPUT_ENCODING);
        printf("<csv>\n");
    }

    // Read and parse input
    linenum = 1;
    first_row = 1;
    for (file_done = 0; !file_done; ) {

        // Start parsing next row
        switch ((ch = readch(fp, 1))) {
        case EOF:
            file_done = 1;
            continue;
        case '\n':                          // ignore completely empty lines
            linenum++;
            continue;
        default:
            ungetc(ch, fp);
            break;
        }

        // Read columns
        while (readcol(fp, &row, &linenum))
            ;

        // Gather column names from first row, if configured
        if (first_row && read_column_names) {
            if (xml)
                convert_to_utf8(icd, &row, linenum);
            memcpy(&column_names, &row, sizeof(row));
            memset(&row, 0, sizeof(row));

            // If we had to defer parsing format string until we had the column names, do that now
            if (!xml)
                nargs = parsefmt(format, &column_names, &args);

            // Proceed
            goto next;
        }

        // Perform XML encoding
        if (xml) {
            int col;

            // Convert columns to UTF-8
            convert_to_utf8(icd, &row, linenum);

            // Output columns for row
            printf("  <row>\n");
            for (col = 0; col < row.num; col++) {
                const char *ptr = row.fields[col];
                int len = strlen(ptr);
                const char *esc;
                int uchar;
                int uclen;
                int i;

                // Open XML tag
                printf("    <");
                if (xml == 2 && col < column_names.num)
                    print_xml_tag_name(column_names.fields[col], linenum);
                else
                    printf("col%d", col + 1);
                printf(">");

                // Output XML characters, escaped as needed
                while (len > 0) {
                    uchar = decode_utf8(ptr, len, &uclen, linenum);
                    if ((esc = escape_xml_char(uchar)) != NULL)
                        printf("%s", esc);
                    else {
                        for (i = 0; i < uclen; i++)
                            putchar(ptr[i]);
                    }
                    ptr += uclen;
                    len -= uclen;
                }

                // Close XML tag
                printf("</");
                if (xml == 2 && col < column_names.num)
                    print_xml_tag_name(column_names.fields[col], linenum);
                else
                    printf("col%d", col + 1);
                printf(">\n");
            }
            printf("  </row>\n");
        } else {
            char ncolbuf[32];
            char empty[] = { '\0' };
            pid_t pid;
            pid_t result;
            int status;
            int i;

            fflush(stdout);
            fflush(stderr);
            switch ((pid = fork())) {
            case -1:
                err(1, "fork");
            case 0:
                close(0);
                if ((argv = malloc((nargs + 3) * sizeof(*argv))) == NULL)
                    err(1, "malloc");
                argv[0] = strdup("printf");
                if (argv[0] == NULL)
                    err(1, "strdup");
                argv[1] = format;
                snprintf(ncolbuf, sizeof(ncolbuf), "%lu", (unsigned long)row.num);
                for (i = 0; i < nargs; i++)
                    argv[2 + i] = args[i] == 0 ? ncolbuf : args[i] <= row.num ? row.fields[args[i] - 1] : empty;
                argv[2 + nargs] = NULL;
                execvp(PRINTF_PROGRAM, argv);
                err(1, "execvp");
            default:
                while (1) {
                    if ((result = waitpid(pid, &status, 0)) == -1)
                        err(1, "waitpid");
                    if (WIFEXITED(status)) {
                        if (WEXITSTATUS(status) != 0)
                            exit(status);
                        break;
                    }
                    if (WIFSIGNALED(status))
                        exit(1);
                }
                break;
            }
        }

next:
        // Free row memory
        freerow(&row);
        first_row = 0;
    }

    // XML closing
    if (xml) {
        printf("</csv>\n");
        (void)iconv_close(icd);
    }

    // Clean up
    if (fp != stdin)
        fclose(fp);
    freerow(&column_names);
    free(args);

    // Done
    fflush(stdout);
    return 0;
}

// Output XML tag name, substituting invalid characters
static void
print_xml_tag_name(const char *tag, int linenum)
{
    int first = 1;
    int uchar;
    int uclen;
    int ok;
    int i;

    while (*tag != '\0') {
        uchar = decode_utf8(tag, strlen(tag), &uclen, linenum);
        if (first) {
            ok = isalpha(uchar) || uchar == '_';
            first = 0;
        } else
            ok = isalpha(uchar) || isdigit(uchar) || uchar == '_' || uchar == '-' || uchar == '.';
        if (!ok)
            putchar('_');
        else {
            for (i = 0; i < uclen; i++)
                putchar(tag[i]);
        }
        tag += uclen;
    }
}

static const char *
escape_xml_char(int uchar)
{
    static char buf[32];

    switch (uchar) {
    case '>':
        return "&gt;";
        break;
    case '<':
        return "&lt;";
        break;
    case '&':
        return "&amp;";
        break;
    case '"':
        return "&quot;";
        break;
    default:

        // Pass valid and unrestricted characters through (but not CR)
        // http://en.wikipedia.org/wiki/Valid_characters_in_XML
        if ((uchar == '\n' || uchar == '\t'
            || (uchar >= 0x0020 && uchar <= 0xd7ff)
            || (uchar >= 0xe000 && uchar <= 0xfffd)
            || (uchar >= 0x10000 && uchar <= 0x10ffff))
          && !((uchar >= 0x007f && uchar <= 0x0084) || (uchar >= 0x0086 && uchar <= 0x009F)))
            return NULL;

        // Escape other characters
        snprintf(buf, sizeof(buf), "&#%u;", uchar);
        return buf;
    }
}

// Convert row columns to UTF-8 encoding
static void
convert_to_utf8(iconv_t icd, struct row *row, int linenum)
{
    int col;

    for (col = 0; col < row->num; col++) {
        char *const ibuf = row->fields[col];
        char *iptr;
        char *obuf;
        char *optr;
        size_t iremain;
        size_t oremain;
        size_t olen;

        // Convert column
        if (iconv(icd, NULL, NULL, NULL, NULL) == (size_t)-1)
            err(1, "iconv");
        iremain = strlen(ibuf);
        oremain = 64 + 4 * iremain;
        if ((obuf = malloc(oremain)) == NULL)
            err(1, "malloc");
        iptr = ibuf;
        optr = obuf;
        if (iconv(icd, &iptr, &iremain, &optr, &oremain) == (size_t)-1) {
            switch (errno) {
            case EILSEQ:
            case EINVAL:
                errx(1, "line %d: invalid multibyte sequence", linenum);
            default:
                err(1, "iconv");
            }
        }
        olen = optr - obuf;

        // Replace column
        if ((row->fields[col] = realloc(row->fields[col], olen + 1)) == NULL)
            err(1, "realloc");
        memcpy(row->fields[col], obuf, olen);
        row->fields[col][olen] = '\0';
        free(obuf);
    }
}

// Decode UTF-8 character
static int
decode_utf8(const char *const obuf, size_t olen, int *lenp, int linenum)
{
    int uchar;
    int uclen;
    int i = 0;

    if ((obuf[i] & 0x80) == 0x00) {
        uclen = 1;
        uchar = obuf[i] & 0x7f;
    } else if ((obuf[i] & 0xe0) == 0xc0 && i + 1 < olen) {
        uclen = 2;
        uchar = ((obuf[i] & 0x1f) <<  6)
          | ((obuf[i + 1] & 0x3f) <<  0);
    } else if ((obuf[i] & 0xf0) == 0xe0 && i + 2 < olen) {
        uclen = 3;
        uchar = ((obuf[i] & 0x0f) << 12)
          | ((obuf[i + 1] & 0x3f) <<  6)
          | ((obuf[i + 2] & 0x3f) <<  0);
    } else if ((obuf[i] & 0xf8) == 0xf0 && i + 3 < olen) {
        uclen = 4;
        uchar = ((obuf[i] & 0x07) << 18)
          | ((obuf[i + 1] & 0x3f) << 12)
          | ((obuf[i + 2] & 0x3f) <<  6)
          | ((obuf[i + 3] & 0x3f) <<  0);
    } else if ((obuf[i] & 0xfc) == 0xf8 && i + 4 < olen) {
        uclen = 5;
        uchar = ((obuf[i] & 0x03) << 24)
          | ((obuf[i + 1] & 0x3f) << 18)
          | ((obuf[i + 2] & 0x3f) << 12)
          | ((obuf[i + 3] & 0x3f) <<  6)
          | ((obuf[i + 4] & 0x3f) <<  0);
    } else if ((obuf[i] & 0xfe) == 0xfc && i + 5 < olen) {
        uclen = 6;
        uchar = ((obuf[i] & 0x01) << 30)
          | ((obuf[i + 1] & 0x3f) << 24)
          | ((obuf[i + 2] & 0x3f) << 18)
          | ((obuf[i + 3] & 0x3f) << 12)
          | ((obuf[i + 4] & 0x3f) <<  6)
          | ((obuf[i + 5] & 0x3f) <<  0);
    } else
        errx(1, "line %d: internal error decoding UTF-8: 0x%02x", linenum, obuf[i] & 0xff);

    // Done
    *lenp = uclen;
    return uchar;
}

static int
readcol(FILE *fp, struct row *row, int *linenum)
{
    struct col col;
    int row_done;
    int ch;

    // Process initial stuff; skip leading whitespace
    do {
        if ((ch = readch(fp, 1)) == EOF)
            ch = '\n';
        if (ch == '\n') {           // end of line forces empty column and terminates the row
            memset(&col, 0, sizeof(col));
            addcolumn(row, &col);
            (*linenum)++;
            return 0;
        }
    } while (isspace(ch));
    ungetc(ch, fp);

    // Read quoted or unquoted value
    if (ch == quote)
        row_done = readqcol(fp, &col, linenum);
    else
        row_done = readuqcol(fp, &col, linenum);
    addcolumn(row, &col);
    return row_done;
}

//
// Read a quoted column, return true if there's more
//
static int
readqcol(FILE *fp, struct col *col, int *linenum)
{
    int done = 0;
    int escape = 0;
    int ch;

    readch(fp, 0);
    memset(col, 0, sizeof(*col));
    while (1) {
        assert(!escape || !done);
        if ((ch = readch(fp, escape)) == EOF) {
            if (escape || done)
                ch = '\n';
            else
                errx(1, "line %d: premature EOF", *linenum);
        }
        if (done) {
            if (ch == '\n') {
                (*linenum)++;
                return 0;
            }
            if (ch == fsep)
                return 1;
            if (isspace(ch))
                continue;
            errx(1, "line %d: unexpected character \"%c\"", *linenum, ch);
        }
        if (escape) {
            if (ch == quote)
                addchar(col, quote);
            else {
                ungetc(ch, fp);
                done = 1;
            }
            escape = 0;
            continue;
        }
        if (ch == quote) {
            escape = 1;
            continue;
        }
        addchar(col, ch);
        if (ch == '\n')
            (*linenum)++;
    }
}

//
// Read an unquoted column, return true if there's more
//
static int
readuqcol(FILE *fp, struct col *col, int *linenum)
{
    int ch;

    memset(col, 0, sizeof(*col));
    while (1) {
        if ((ch = readch(fp, 1)) == EOF)
            ch = '\n';
        if (ch == '\n') {
            (*linenum)++;
            trim(col);
            return 0;
        }
        if (ch == fsep) {
            trim(col);
            return 1;
        }
        addchar(col, ch);
    }
}

//
// Trims whitespace around a column
//
static void
trim(struct col *col)
{
    size_t skip;

    while (col->len > 0 && isspace(col->buf[col->len - 1]))
        col->len--;
    for (skip = 0; skip < col->len && isspace(col->buf[skip]); skip++)
        ;
    col->len -= skip;
    memmove(col->buf, col->buf + skip, col->len);
}

//
// Adds the character to the column
//
static void
addchar(struct col *col, int ch)
{
    if (col->alloc <= col->len) {
        int new_alloc;
        char *new_buf;

        new_alloc = col->alloc == 0 ? 32 : col->alloc * 2;
        if ((new_buf = realloc(col->buf, new_alloc)) == NULL)
            err(1, "realloc");
        col->buf = new_buf;
        col->alloc = new_alloc;
    }
    col->buf[col->len++] = ch;
}

//
// Adds the column to the row, then frees the column
//
static void
addcolumn(struct row *row, const struct col *col)
{
    if (row->alloc <= row->num) {
        int new_alloc;
        char **new_fields;

        new_alloc = row->alloc == 0 ? 32 : row->alloc * 2;
        if ((new_fields = realloc(row->fields, new_alloc * sizeof(*row->fields))) == NULL)
            err(1, "realloc");
        row->fields = new_fields;
        row->alloc = new_alloc;
    }
    if (col->alloc >= col->len + 1) {
        col->buf[col->len] = '\0';
        row->fields[row->num] = col->buf;
        memset(&col, 0, sizeof(col));
    } else {
        if ((row->fields[row->num] = malloc(col->len + 1)) == NULL)
            err(1, "malloc");
        memcpy(row->fields[row->num], col->buf, col->len);
        row->fields[row->num][col->len] = '\0';
        free(col->buf);
    }
    memset(&col, 0, sizeof(col));
    row->num++;
}

static int
parsefmt(char *fmt, const struct row *column_names, unsigned int **argsp)
{
    unsigned int *args;
    int nargs;
    int alloc;
    char *s;

    // Size and allocate array
    alloc = 0;
    for (s = fmt; *s != '\0'; s++) {
        if (*s == '%')
            alloc += 3;
    }
    if ((args = malloc(alloc * sizeof(*args))) == NULL)
        err(1, "malloc");
    nargs = 0;

    // Parse format
    for (s = fmt; *s != '\0'; s++) {
        char *const fspec = s;
        if (*s != '%' || *++s == '%')
            continue;
        s = eataccessor(fspec, "format specification", column_names, s, &nargs, args);
        while (*s != '\0' && strchr("#-+ 0", *s) != NULL)       // eat up optional flags
            s++;
        s = eatwidthprec(fspec, "field width for format specification", column_names, s, &nargs, args);
        if (*s == '.')
            s = eatwidthprec(fspec, "precision for format specification", column_names, s + 1, &nargs, args);
        if (*s == '\0')
            errx(1, "truncated format specification starting at \"%.20s...\"", fspec);
    }

    // Done
    *argsp = args;
    return nargs;
}

static int
parsechar(const char *str)
{
    char *eptr;
    int ch;

    switch (strlen(str)) {
    case 1:
        ch = (unsigned char)*str;
        break;
    case 2:
        if (*str != '\\')
            return -1;
        switch (str[1]) {
        case 'a':
            ch = '\a';
            break;
        case 't':
            ch = '\t';
            break;
        case 'b':
            ch = '\b';
            break;
        case 'r':
            ch = '\r';
            break;
        case 'f':
            ch = '\f';
            break;
        case 'v':
            ch = '\v';
            break;
        case '\\':
        case '\'':
        case '"':
            ch = str[1];
            break;
        default:
            return -1;
        }
        break;
    case 4:
        if (*str != '\\')
            return -1;
        ch = str[1] == 'x' ? strtoul(str + 2, &eptr, 16) : strtoul(str + 1, &eptr, 8);
        if (*eptr != '\0')
            return -1;
        break;
    default:
        return -1;
    }

    // Disallow line separator
    if (ch == '\n')
        return -1;

    // Disallow overflown values
    if (ch != (ch & 0xff))
        return -1;

    // Done
    return ch;
}

static char *
eatwidthprec(const char *const fspec, const char *desc, const struct row *column_names, char *s, int *nargs, unsigned int *args)
{
    if (*s == '*')
        return eataccessor(fspec, desc, column_names, s + 1, nargs, args);
    while (isdigit(*s))                                         // eat up numerical field width or precision
        s++;
    return s;
}

static char *
eataccessor(const char *const fspec, const char *desc, const struct row *column_names, char *s, int *nargs, unsigned int *args)
{
    char *const start = s;
    const char *colname;
    int namelen;
    int argnum;
    int i;

    if (*s == '{') {
        if (column_names == NULL)
            errx(1, "symbolic column accessors require \"-i\" flag in %s starting at \"%.20s...\"", desc, fspec);
        colname = ++s;
        while (*s != '}') {
            if (*s++ == '\0')
                errx(1, "malformed column accessor in %s starting at \"%.20s...\"", desc, fspec);
        }
        namelen = s++ - colname;
        argnum = 0;
        for (i = 0; i < column_names->num; i++) {
            if (strncmp(colname, column_names->fields[i], namelen) == 0 && column_names->fields[i][namelen] == '\0') {
                if (argnum != 0) {
                    errx(1, "ambiguous column name \"%.*s\" in symbolic column accessor in %s starting at \"%.20s...\"",
                      namelen, colname, desc, fspec);
                }
                argnum = i + 1;
            }
        }
        if (argnum == 0) {
            errx(1, "unknown column name \"%.*s\" in symbolic column accessor in %s starting at \"%.20s...\"",
              namelen, colname, desc, fspec);
        }
        args[(*nargs)++] = argnum;
    } else {
        while (isdigit(*s))
            s++;
        if (s == start || *s++ != '$')
            errx(1, "missing required column accessor in %s starting at \"%.20s...\"", desc, fspec);
        sscanf(start, "%u", &args[(*nargs)++]);
    }
    memmove(start, s, strlen(s) + 1);
    return start;
}

// Like getc() but optionally collapses CR or CR, LF into a single LF
static int
readch(FILE *fp, int collapse)
{
    int ch;

    ch = getc(fp);
    if (collapse && ch == '\r') {
        if ((ch = getc(fp)) != '\n') {
            ungetc(ch, fp);
            ch = '\n';
        }
    }
    return ch;
}

static void
freerow(struct row *row)
{
    while (row->num > 0)
        free(row->fields[--row->num]);
    free(row->fields);
    memset(row, 0, sizeof(*row));
}

static void
usage(void)
{

    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  csvprintf [options] format\n");
    fprintf(stderr, "  csvprintf -x [options]\n");
    fprintf(stderr, "  csvprintf -X [options]\n");
    fprintf(stderr, "  csvprintf -h\n");
    fprintf(stderr, "  csvprintf -v\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -e encoding\tSpecify input character encoding (XML mode only; default ISO-8859-1)\n");
    fprintf(stderr, "  -f input\tRead CSV input from specified file (default stdin)\n");
    fprintf(stderr, "  -i\t\tAssume the first CSV record contains column names\n");
    fprintf(stderr, "  -q char\tSpecify quote character (default `%c')\n", DEFAULT_QUOTE_CHAR);
    fprintf(stderr, "  -s char\tSpecify field separator character (default `%c')\n", DEFAULT_FSEP_CHAR);
    fprintf(stderr, "  -x\t\tConvert input to XML using numeric tags\n");
    fprintf(stderr, "  -X\t\tConvert input to XML using column name tags (implies \"-i\")\n");
    fprintf(stderr, "  -h\t\tOutput this help message and exit\n");
    fprintf(stderr, "  -v\t\tOutput version information and exit\n");
}

static void
version(void)
{
    fprintf(stderr, "%s version %s (%s)\n", PACKAGE_TARNAME, PACKAGE_VERSION, csvprintf_version);
    fprintf(stderr, "Copyright (C) 2010 Archie L. Cobbs\n");
    fprintf(stderr, "This is free software; see the source for copying conditions. There is NO\n");
    fprintf(stderr, "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
}

