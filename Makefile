EXCLUDE_WARNINGS = \
    -Wno-gnu-conditional-omitted-operand \
    -Wno-gnu-designator \
    -Wno-gnu-statement-expression

CFLAGS := -std=gnu11 -pedantic -Wall -Werror $(EXCLUDE_WARNINGS)
NAME = interp

SRCDIR := ./src
OBJDIR := ./obj
SRCS := $(addprefix $(SRCDIR)/, lex.c parse.c run.c main.c)
OBJS := $(addprefix $(OBJDIR)/, $(notdir $(SRCS:.c=.o)))

all: $(NAME)

$(OBJS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) -o $(NAME) $^

.PHONY: clean

clean:
	rm -rf $(OBJDIR)
	rm -f $(NAME)
