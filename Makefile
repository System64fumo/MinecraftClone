SRCS +=	$(wildcard src/*.c)
OBJS = $(patsubst src/%,$(BUILDDIR)/%,$(SRCS:.c=.o))
CFLAGS=-mtune=native -march=native
BUILDDIR = build

$(shell mkdir -p $(BUILDDIR))

all: release

release: CFLAGS += -Oz -s -flto -Wl,--gc-sections -ffunction-sections -fdata-sections -ffast-math
release: game
	@which upx >/dev/null 2>&1 && upx --best --lzma $(BUILDDIR)/game || true

debug: CFLAGS += -O0 -g -Wall -DDEBUG
debug: game

clean:
	rm -rf $(BUILDDIR)

game: $(OBJS)
	$(CC) -o $(BUILDDIR)/game $(OBJS) $(CFLAGS) -lGL -lGLU -lGLEW -lglfw -lglut -lm

$(BUILDDIR)/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)