NAME    = ft_ping

CC      = gcc
CFLAGS  = -Wall -Wextra -Werror

SRCS    = main.c parsing.c utils.c handle_flag.c icmp.c icmp_reply.c
OBJS    = $(addprefix objs/, $(SRCS:.c=.o))

all: $(NAME)

objs/%.o: %.c
	mkdir -p objs
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -rf objs

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re