SRCS +=	$(wildcard src/*.c)
OBJS = $(patsubst src/%,$(BUILDDIR)/%,$(SRCS:.c=.o))
CFLAGS=-mtune=native -march=native
BUILDDIR = build

$(shell mkdir -p $(BUILDDIR))

all: release

release: CFLAGS = $(CFLAGS_DEBUG) -Ofast -s 
release: game

debug: CFLAGS = $(CFLAGS_DEBUG) -O0 -g -DDEBUG
debug: game

clean:
	rm -rf $(BUILDDIR)

game: $(OBJS)
	$(CC) -o $(BUILDDIR)/game $(OBJS) $(CFLAGS) -lSDL2 -lGL -lGLU -lGLEW -lglut -Wall -lm

$(BUILDDIR)/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)
