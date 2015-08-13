CFLAGS = -std=gnu99 -Wall -Werror
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
