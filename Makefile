CC = gcc
CFLAGS = -I. -Wall -O2
LDFLAGS = -libverbs

TARGET = irdma_test
SRCS = irdma_test.c pingpong.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(TARGET) $(OBJS)