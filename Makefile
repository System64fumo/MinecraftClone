SRCS +=	$(wildcard src/*.c) $(wildcard src/mesh/*.c) 
OBJS = $(patsubst src/%, $(BUILDDIR)/%, $(patsubst src/mesh/%, $(BUILDDIR)/%, $(SRCS:.c=.o)))
CFLAGS = -mtune=native -march=native -Ofast -flto -Wl,--gc-sections -ffunction-sections -fdata-sections
CFLAGS += -I include
BUILDDIR = build
VPATH = src src/mesh

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

$(BUILDDIR)/%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)
