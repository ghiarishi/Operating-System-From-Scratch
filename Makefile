TARGETS = pennfat # pennos
CC = clang

OSSRCS =
FSSRCS = src/pennfat.c $(wildcard src/fs/**.c)

OSOBJS = $(OSSRCS:.c=.o)
FSOBJS = $(FSSRCS:.c=.o)

# Replace -O1 with -g for a debug version during development
# CFLAGS = -Wall -Werror -O1
CFLAGS = -Wall -Werror -g

all: $(TARGETS)

pennos: $(OSOBJS)
	$(CC) -o bin/$@ $^

pennfat: $(FSOBJS)
	$(CC) -o bin/$@ $^

clean:
	$(RM) $(OSOBJS) $(FSOBJS) $(TARGETS)
