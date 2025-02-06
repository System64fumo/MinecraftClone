SRCS +=	$(wildcard src/*.c)
OBJS = $(patsubst src/%,$(BUILDDIR)/%,$(SRCS:.c=.o))
CFLAGS=-Ofast -s -mtune=native -march=native
BUILDDIR = build

$(shell mkdir -p $(BUILDDIR))

all: game

clean:
	rm -rf $(BUILDDIR)

game: $(OBJS)
	$(CC) -o $(BUILDDIR)/game $(OBJS) $(CFLAGS) -lSDL2 -lSDL2_image -lSDL2_ttf -lGL -lGLU -lglut -Wall -lm

$(BUILDDIR)/%.o: src/%.c
	$(CXX) -c $< -o $@ $(CFLAGS)
