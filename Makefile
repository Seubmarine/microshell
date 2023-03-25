NAME = microshell
CC = cc
CFLAGS = -g3 -Wall -Wextra -Werror

SRCS = main.c

OBJS = $(SRCS:.c=.o)


all: $(NAME)

%.o: %.c
	$(CC) $(CFLAGS) $< -c -o $@

clean :
	-rm -f $(OBJS)

fclean : clean
	-rm -f $(NAME)

re : fclean $(NAME)

$(NAME) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $(NAME)

.PHONY: all clean fclean re