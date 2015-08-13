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

struct token tokens[400];
size_t ntokens;

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
    int lexed = lex(mapped, tokens, &ntokens);

    for (size_t i = 0, alternate = 0; i < ntokens; ++i) {
        struct token token = tokens[i];

        if (token.tk == TK_FBEG || token.tk == TK_FEND) {
            continue;
        }

        if (token.tk != TK_WSPC && token.tk != TK_LCOM && token.tk != TK_BCOM) {
            alternate++;
        }

        int len = token.end - token.beg;

        if (i == ntokens - 1 && !lexed) {
            printf(RED("%.*s") CYAN(" < Unknown token\n"), len ?: 1, token.beg);
        } else if (token.tk == TK_LCOM || token.tk == TK_BCOM) {
            printf(GRAY("%.*s"), len, token.beg);
        } else if (alternate % 2) {
            printf(GREEN("%.*s"), len, token.beg);
        } else {
            printf(YELLOW("%.*s"), len, token.beg);
        }
    }

    if (lexed) {
        puts(WHITE("\n*** Parsing ***"));
        struct node root = parse(tokens, ntokens);

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
