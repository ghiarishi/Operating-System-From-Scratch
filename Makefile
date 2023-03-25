TARGETS = pennfat pennos
CC = clang

# define sources
SRCS = $(wildcard src/common/**.c) $(wildcard src/fs/**.c)
OSSRCS = src/pennos.c $(SRCS)
FSSRCS = src/pennfat.c $(SRCS) $(wildcard src/pennfat/**.c)

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
