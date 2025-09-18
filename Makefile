CC = gcc

CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic

TARGET = reboobs

all: $(TARGET)

$(TARGET): main.c
				$(CC) $(CFLAGS) main.c -o $(TARGET)

clean:
		rm -f reboobs *.o
