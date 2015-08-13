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

static void print_tokens(const struct token *tokens, size_t ntokens, int error)
{
    for (size_t i = 0, alternate = 0; i < ntokens; ++i) {
        struct token token = tokens[i];

        if (token.tk == TK_FBEG || token.tk == TK_FEND) {
            continue;
        }

        if (token.tk != TK_WSPC && token.tk != TK_LCOM && token.tk != TK_BCOM) {
            alternate++;
        }

        int len = token.end - token.beg;

        if (i == ntokens - 1 && error == LEX_UNKNOWN_TOKEN) {
            printf(RED("%.*s") CYAN(" < Unknown token\n"), len ?: 1, token.beg);
        } else if (token.tk == TK_LCOM || token.tk == TK_BCOM) {
            printf(GRAY("%.*s"), len, token.beg);
        } else if (alternate % 2) {
            printf(GREEN("%.*s"), len, token.beg);
        } else {
            printf(YELLOW("%.*s"), len, token.beg);
        }
    }
}

int main(int argc, char **argv)
{
    int fd;
    size_t size;
    struct stat statbuf;
    int exit_status = EXIT_FAILURE;

    if (argc != 2) {
        return fprintf(stderr, "Usage: %s <file>\n", argv[0]), exit_status;
    }

    if ((fd = open(argv[1], O_RDONLY)) < 0) {
        return perror("open"), exit_status;
    }

    if (fstat(fd, &statbuf) < 0) {
        return perror("fstat"), close(fd), exit_status;
    }

    if ((size = statbuf.st_size) == 0) {
        fprintf(stderr, "‘%s‘: file is empty\n", argv[1]);
        return close(fd), exit_status;
    }

    const uint8_t *mapped = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (mapped == MAP_FAILED) {
        return perror("mmap"), close(fd), exit_status;
    }

    puts(WHITE("*** Lexing ***"));
    struct token *tokens;
    size_t ntokens;
    int lex_error = lex(mapped, &tokens, &ntokens);

    if (!lex_error || lex_error == LEX_UNKNOWN_TOKEN) {
        print_tokens(tokens, ntokens, lex_error);
    } else if (lex_error == LEX_NOMEM) {
        puts(RED("The lexer could not allocate memory."));
    }

    if (!lex_error) {
        puts(WHITE("\n*** Parsing ***"));
        struct node root = parse(tokens, ntokens);

        if (!parse_error(root)) {
            puts(WHITE("\n*** Running ***"));
            run(&root);
            destroy_tree(root);
            exit_status = EXIT_SUCCESS;
        }
    }

    free(tokens);
    munmap((uint8_t *) mapped, size);
    close(fd);
    return exit_status;
}