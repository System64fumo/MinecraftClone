EXECUTABLE = game
SRC_DIRS := src src/graphics src/mesh src/world
BUILDDIR := build
INCLUDE_DIR := include

CFLAGS := -I$(INCLUDE_DIR)
LDFLAGS := -lGL -lGLEW -lglfw -lm -lpng

SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
OBJS := $(patsubst %.c, $(BUILDDIR)/%.o, $(SRCS))

$(shell mkdir -p $(BUILDDIR))
$(foreach dir, $(SRC_DIRS), $(shell mkdir -p $(BUILDDIR)/$(dir)))

VPATH := $(SRC_DIRS)

JOB_COUNT := $(EXECUTABLE) $(OBJS)
JOBS_DONE := $(shell ls -l $(JOB_COUNT) 2> /dev/null | wc -l)

define progress
	$(eval JOBS_DONE := $(shell echo $$(($(JOBS_DONE) + 1))))
	@printf "[$(JOBS_DONE)/$(shell echo $(JOB_COUNT) | wc -w)] %s %s\n" $(1) $(2)
endef

all: release

release: CFLAGS += -mtune=native -march=native -flto -Ofast -pipe -s -fno-stack-protector -fomit-frame-pointer -ffunction-sections -fdata-sections -fmerge-all-constants -fno-ident
release: LDFLAGS += -Wl,--gc-sections -Wl,-z,norelro
release: JOB_COUNT += "Compression"
release: game
	$(call progress, Compressing $(EXECUTABLE))
	@which upx >/dev/null 2>&1 && upx --best --lzma $(BUILDDIR)/$(EXECUTABLE) >/dev/null 2>&1 || true

debug: CFLAGS += -g -Wall -DDEBUG
debug: game

clean:
	rm -rf $(BUILDDIR)

game: $(OBJS)
	$(call progress, Linking $@)
	@$(CC) -o $(BUILDDIR)/$(EXECUTABLE) $(OBJS) $(CFLAGS) $(LDFLAGS)

$(BUILDDIR)/%.o: %.c
	$(call progress, Compiling $@)
	@$(CC) -c $< -o $@ $(CFLAGS)
