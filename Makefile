CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -D_POSIX_C_SOURCE=200809L

TARGET = s21_grep
SRCS = src/main.c src/grep.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
