#include "main.h"
#include "framebuffer.h"
#include "shaders.h"
#include "config.h"
#include "gui.h"
#include <stdio.h>
#include <string.h>
#include "xdg-shell-client-protocol.h"

struct wl_display *display;
struct wl_compositor *compositor;
struct wl_surface *surface;
struct xdg_wm_base *xdg_wm_base;
struct xdg_surface *xdg_surface;
struct xdg_toplevel *xdg_toplevel;
struct wl_seat *seat;
struct wl_touch *touch;

float near = 0.1f;
float far = 500.0f;
float aspect = 1.7f;
GLFWwindow* window = NULL;
mat4 model, view, projection;
unsigned short screen_center_x = 640;
unsigned short screen_center_y = 360;
bool game_focused = true;

static void touch_frame(void *data, struct wl_touch *wl_touch) {
}

static void touch_cancel(void *data, struct wl_touch *wl_touch) {
	printf("Touch cancel\n");
}

static const struct wl_touch_listener touch_listener = {
	.down = touch_down,
	.up = touch_up,
	.motion = touch_motion,
	.frame = touch_frame,
	.cancel = touch_cancel,
};

static void seat_handle_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
	if (!(capabilities & WL_SEAT_CAPABILITY_TOUCH))
		return;

	touch = wl_seat_get_touch(seat);
	wl_touch_add_listener(touch, &touch_listener, NULL);
}

static const struct wl_seat_listener seat_listener = {
	.capabilities = seat_handle_capabilities,
};

static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
	xdg_wm_base_pong(xdg_wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};

static void registry_handler(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
	if (strcmp(interface, "wl_compositor") == 0) {
		compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	}
	else if (strcmp(interface, "xdg_wm_base") == 0) {
		xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
	}
	else if (strcmp(interface, "wl_seat") == 0) {
		seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
		wl_seat_add_listener(seat, &seat_listener, NULL);
	}
}

static void registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handler,
	.global_remove = registry_remover,
};

void window_focus_callback(GLFWwindow* window, int focused) {
	game_focused = focused;
}

int initialize_window() {
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_ANY_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
	#ifndef DEBUG
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	#endif
	//glfwWindowHint(GLFW_SAMPLES, 8);

	GLFWmonitor* monitor = NULL;
	if (settings.fullscreen)
		monitor = glfwGetPrimaryMonitor();

	window = glfwCreateWindow(settings.window_width, settings.window_height, "Minecraft Clone", monitor, NULL);
	if (!window) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	surface = glfwGetWaylandWindow(window);
	display = glfwGetWaylandDisplay();

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetKeyCallback(window, key_callback);
	glfwSetWindowFocusCallback(window, window_focus_callback);

	glewInit();

	// Debugging
	#ifdef DEBUG
	profiler_init();
	profiler_create("Shaders");
	profiler_create("Mesh");
	profiler_create("Merge");
	profiler_create("Render");
	profiler_create("GUI");
	profiler_create("Culling");
	profiler_create("Framebuffer");
	profiler_create("World");
	#endif

	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// TODO: Write changes back to config file
	settings.window_width = width;
	settings.window_height = height;

	aspect = (float)settings.window_width / (float)settings.window_height;
	screen_center_x = settings.window_width / 2;
	screen_center_y = settings.window_height / 2;

	glViewport(0, 0, settings.window_width, settings.window_height);
	matrix4_identity(projection);
	matrix4_perspective(projection, settings.fov * DEG_TO_RAD, aspect, near, far);
	setup_framebuffer(width, height);
	update_ui();

	load_shader_constants();
}

void set_fov(float fov) {
	matrix4_perspective(projection, fov * DEG_TO_RAD, aspect, near, far);
}