PROMPT='"$ "'

TARGETS = pennfat pennos
CC = clang

# define sources
SRCS = $(wildcard src/common/**.c) $(wildcard src/fs/**.c)
OSSRCS = src/pennos.c $(SRCS) $(wildcard src/process/**.c) # src/process/scheduler.c src/process/pcb.c
FSSRCS = src/pennfat.c $(wildcard src/common/**.c) src/fs/fat.c $(wildcard src/pennfat/**.c)

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
