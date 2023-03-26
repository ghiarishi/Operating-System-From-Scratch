TARGETS = pennfat pennos
CC = clang

# define sources
SRCS = $(wildcard src/common/**.c) $(wildcard src/fs/**.c)
OSSRCS = src/pennos.c $(SRCS)
FSSRCS = src/pennfat.c $(SRCS) $(wildcard src/pennfat/**.c)
SCHEDSRCS = src/scheduler.c $(SRCS)

#  

OSOBJS = $(OSSRCS:.c=.o)
FSOBJS = $(FSSRCS:.c=.o)
SCHEDOBJS = $(SCHEDSRCS:.c=.o)


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

scheduler: $(SCHEDOBJS)
	$(CC) -o $(BIN)/$@ $^

clean:
	$(RM) $(OSOBJS) $(FSOBJS) $(SCHEDOBJS) $(TARGETS:%=$(BIN)/%)
