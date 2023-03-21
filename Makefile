TARGETS = pennfat pennos
CC = clang

# define sources
OSSRCS = src/pennos.c
FSSRCS = src/pennfat.c $(wildcard src/fs/**.c)

OSOBJS = $(OSSRCS:.c=.o)
FSOBJS = $(FSSRCS:.c=.o)

# output dir
BIN = bin

# Replace -O1 with -g for a debug version during development
# CFLAGS = -Wall -Werror -O1
CFLAGS = -Wall -Werror -g

all: $(TARGETS)

pennos: $(OSOBJS)
	$(CC) -o $(BIN)/$@ $^

pennfat: $(FSOBJS)
	$(CC) -o $(BIN)/$@ $^

clean:
	$(RM) $(OSOBJS) $(FSOBJS) $(TARGETS:%=$(BIN)/%)
