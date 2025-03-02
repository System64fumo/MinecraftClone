#include "main.h"
#include "engine.h"
#include "framebuffer.h"

int main() {
	if (initialize() != 0) {
		return -1;
	}
	run();
	shutdown();

	return 0;
}