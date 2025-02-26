SRC_DIRS := src src/mesh src/world
BUILDDIR := build
INCLUDE_DIR := include

CFLAGS := -mtune=native -march=native -Ofast -flto -Wl,--gc-sections -ffunction-sections -fdata-sections
CFLAGS += -I$(INCLUDE_DIR)

SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
OBJS := $(patsubst %.c, $(BUILDDIR)/%.o, $(SRCS))

$(shell mkdir -p $(BUILDDIR))
$(foreach dir, $(SRC_DIRS), $(shell mkdir -p $(BUILDDIR)/$(dir)))

VPATH := $(SRC_DIRS)

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