CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lm
TARGET = ride_dispatch

SRCS = main.c \
       src/globals.c \
       src/person1_core.c \
       src/person2_spatial_queries.c \
       src/person3_knn.c \
       src/person4_status_dispatch.c \
       src/person5_diagnostics_performance.c \
       src/menu.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET) ride_dispatch.exe

run: all
	./$(TARGET)

windows:
	$(CC) $(CFLAGS) -o ride_dispatch.exe $(SRCS) $(LDFLAGS)
