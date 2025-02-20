SRCS +=	$(wildcard src/*.c)
OBJS = $(patsubst src/%,$(BUILDDIR)/%,$(SRCS:.c=.o))
CFLAGS=-mtune=native -march=native -Ofast -flto -Wl,--gc-sections -ffunction-sections -fdata-sections
BUILDDIR = build

$(shell mkdir -p $(BUILDDIR))

all: release

release: CFLAGS += -s
release: game
	@which upx >/dev/null 2>&1 && upx --best --lzma $(BUILDDIR)/game || true

debug: CFLAGS += -g -Wall -DDEBUG
debug: game

clean:
	rm -rf $(BUILDDIR)

game: $(OBJS)
	$(CC) -o $(BUILDDIR)/game $(OBJS) $(CFLAGS) -lGL -lGLEW -lglfw -lm -lpng

$(BUILDDIR)/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS)
