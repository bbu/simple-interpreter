#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "lex.h"
#include "parse.h"
#include "run.h"

struct token ranges[400];
size_t nranges;

int main(int argc, char **argv)
{
    int fd;
    size_t size;
    struct stat statbuf;

    if (argc != 2) {
        return fprintf(stderr, "Usage: %s <file>\n", argv[0]), EXIT_FAILURE;
    }

    if ((fd = open(argv[1], O_RDONLY)) < 0) {
        return perror("open"), EXIT_FAILURE;
    }

    if (fstat(fd, &statbuf) < 0) {
        return perror("fstat"), close(fd), EXIT_FAILURE;
    }

    if ((size = statbuf.st_size) == 0) {
        fprintf(stderr, "‘%s‘: file is empty\n", argv[1]);
        return close(fd), EXIT_FAILURE;
    }

    const uint8_t *mapped = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (mapped == MAP_FAILED) {
        return perror("mmap"), close(fd), EXIT_FAILURE;
    }

    puts(WHITE("*** Lexing ***"));
    int lexed = lex(mapped, ranges, &nranges);

    for (size_t i = 0, alternate = 0; i < nranges; ++i) {
        struct token range = ranges[i];

        if (range.tk == TK_FBEG || range.tk == TK_FEND) {
            continue;
        }

        if (range.tk != TK_WSPC) {
            alternate++;
        }

        int len = range.end - range.beg;

        if (i == nranges - 1 && !lexed) {
            printf(RED("%.*s") CYAN(" < Unknown token\n"), len ?: 1, range.beg);
        } else if (alternate % 2) {
            printf(GREEN("%.*s"), len, range.beg);
        } else {
            printf(YELLOW("%.*s"), len, range.beg);
        }
    }

    if (lexed) {
        puts(WHITE("\n*** Parsing ***"));
        struct node root = parse(ranges, nranges);

        if (parse_success(root)) {
            puts(WHITE("\n*** Running ***"));
            run(&root);
            destroy_tree(root);
        }
    }

    munmap((uint8_t *) mapped, size);
    close(fd);
    return EXIT_SUCCESS;
}
