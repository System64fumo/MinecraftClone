#include "main.h"

// Mouse control variables
float lastX = 0, lastY = 0;
bool firstMouse = true;

void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.5f, 0.6f, 1.0f, 1.0f);
	glLoadIdentity();

	// Apply camera transformations based on player position
	float lookX = global_entities[0].x - sinf(global_entities[0].yaw * M_PI / 180.0f) * cosf(global_entities[0].pitch * M_PI / 180.0f);
	float lookY = global_entities[0].y + sinf(global_entities[0].pitch * M_PI / 180.0f);
	float lookZ = global_entities[0].z + cosf(global_entities[0].yaw * M_PI / 180.0f) * cosf(global_entities[0].pitch * M_PI / 180.0f);
	gluLookAt(global_entities[0].x, global_entities[0].y, global_entities[0].z,
		  lookX, lookY, lookZ,
		  0.0f, 1.0f, 0.0f);

	// Render world
	render_chunks();

	// Draw HUD with average FPS
	#ifdef DEBUG
	profiler_start(PROFILER_ID_HUD);
	#endif
	draw_hud(fps_average, &global_entities[0]);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_HUD);
	#endif
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	screen_width = width;
	screen_height = height;

	float aspect = (float)screen_width / (float)screen_height;
	float fovRad = (fov * M_PI) / 180.0f;
	float tanHalf = tanf(fovRad / 2.0f);
	screen_center_x = screen_width / 2.0f;
	screen_center_y = screen_height / 2.0f;

	glViewport(0, 0, screen_width, screen_height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-near * aspect * tanHalf, near * aspect * tanHalf, -near * tanHalf, near * tanHalf, near, far);
	glMatrixMode(GL_MODELVIEW);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		switch (key) {
			case GLFW_KEY_R:
				load_around_entity(&global_entities[0]);
				break;
		}
	}
}

void processInput(GLFWwindow* window) {
	float moveSpeed = global_entities[0].speed * deltaTime;
	float yaw = global_entities[0].yaw * (M_PI / 180.0f);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		global_entities[0].x += sinf(-yaw) * moveSpeed;
		global_entities[0].z += cosf(-yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		global_entities[0].x -= sinf(-yaw) * moveSpeed;
		global_entities[0].z -= cosf(-yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		global_entities[0].x += cosf(yaw) * moveSpeed;
		global_entities[0].z += sinf(yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		global_entities[0].x -= cosf(yaw) * moveSpeed;
		global_entities[0].z -= sinf(yaw) * moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
		global_entities[0].y += moveSpeed;
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
		global_entities[0].y -= moveSpeed;
	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    global_entities[0].yaw += xoffset;
    global_entities[0].pitch += yoffset;

	// Clamp pitch
	if (global_entities[0].pitch > 89.0f) {
		global_entities[0].pitch = 89.0f;
	}
	if (global_entities[0].pitch < -89.0f) {
		global_entities[0].pitch = -89.0f;
	}

	// Wrap yaw
	if (global_entities[0].yaw >= 360.0f) {
		global_entities[0].yaw -= 360.0f;
	}
	if (global_entities[0].yaw < 0.0f) {
		global_entities[0].yaw += 360.0f;
	}
}