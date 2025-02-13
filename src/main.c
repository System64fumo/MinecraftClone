#include "main.h"

unsigned short screen_width = 1280;
unsigned short screen_height = 720;
unsigned short screen_center_x = 640;
unsigned short screen_center_y = 360;

float fov = 70.0f;
float near = 0.1f;
float far = 300.0f;
float aspect = 1.7f;

float deltaTime = 0.0f;
float model[16], view[16], projection[16];
unsigned int shaderProgram;

Chunk chunks[RENDERR_DISTANCE][WORLD_HEIGHT][RENDERR_DISTANCE];
Entity global_entities[MAX_ENTITIES_PER_CHUNK * RENDERR_DISTANCE * CHUNK_SIZE];

// Vertex shader source
const char* vertexShaderSource = "#version 330 core\n"
	"layout (location = 0) in vec3 aPos;\n"
	"layout (location = 1) in int inBlockID;\n"
	"layout (location = 2) in int inFaceID;\n"
	"uniform mat4 model;\n"
	"uniform mat4 view;\n"
	"uniform mat4 projection;\n"
	"flat out int blockID;\n"
	"flat out int faceID;\n"
	"void main() {\n"
	"    gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
	"    blockID = inBlockID;\n"
	"    faceID = inFaceID;\n"
	"}\0";


// Fragment shader
const char* fragmentShaderSource = "#version 330 core\n"
	"out vec4 FragColor;\n"
	"flat in int blockID;\n"
	"flat in int faceID;\n"

	"vec3 getBlockColor(int id) {\n"
	"    if (id == 1) return vec3(0.6, 0.4, 0.2);  // Dirt\n"
	"    if (id == 2) return vec3(0.2, 0.8, 0.2);  // Grass\n"
	"    if (id == 3) return vec3(0.5, 0.5, 0.5);  // Stone\n"
	"    return vec3(0.5, 0.5, 0.5);  // Default (Gray for unknown)\n"
	"}\n"

	"vec3 getFaceShade(int id) {\n"
	"    if (id == 0) return vec3(1.0, 1.0, 1.0); // Front (No shading)\n"
	"    if (id == 1) return vec3(0.9, 0.9, 0.9); // Left (Slight shading)\n"
	"    if (id == 2) return vec3(0.85, 0.85, 0.85); // Back (More shading)\n"
	"    if (id == 3) return vec3(0.95, 0.95, 0.95); // Right (Slight lightening)\n"
	"    if (id == 4) return vec3(0.8, 0.8, 0.8); // Bottom (More shading)\n"
	"    if (id == 5) return vec3(1.0, 1.0, 1.0); // Top (No shading)\n"
	"    return vec3(0.9, 0.9, 0.9);  // Default shade (gray)\n"
	"}\n"

	"void main() {\n"
	"    vec3 baseColor = getBlockColor(blockID);\n"
	"    vec3 faceShade = getFaceShade(faceID);\n"
	"    FragColor = vec4(baseColor * faceShade, 1.0); // Apply face shading\n"
	"}\n";



int main() {
	// Initialize GLFW
	if (!glfwInit()) {
		printf("Failed to initialize GLFW\n");
		return -1;
	}

	// Configure GLFW
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window
	GLFWwindow* window = glfwCreateWindow(screen_width, screen_height, "Minecraft Clone", NULL, NULL);
	if (!window) {
		printf("Failed to create GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	glewInit();

	// Create and compile shaders
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	// Create shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	float lastFrame = 0.0f;

	// Initialize player
	Entity player = {
		.x = (WORLD_SIZE * CHUNK_SIZE) / 2.0f,
		.y = 35.0f,
		.z = (WORLD_SIZE * CHUNK_SIZE) / 2.0f,
		.yaw = 0.0f,
		.pitch = 0.0f,
		.speed = 20
	};
	global_entities[0] = player;

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	load_around_entity(&player);

	// Initialize mouse capture
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPos(window, screen_center_x, screen_center_y);
	glfwSetKeyCallback(window, key_callback);

	// Main render loop
	while (!glfwWindowShouldClose(window)) {
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Process input
		processInput(window);

		// Clear buffers
		glClearColor(0.471f, 0.655f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use shader program
		glUseProgram(shaderProgram);

		// Set up matrices
		setupMatrices(view, projection);

		// Set view and projection uniforms
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
		glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection);

		// Render chunks
		render_chunks();

		// Swap buffers and poll events
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	cleanup();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}

void cleanup() {
	// for (int x = 0; x < 16; x++) {
	//     for (int y = 0; y < 16; y++) {
	//         for (int z = 0; z < 16; z++) {
	//             drawChunk(&chunks[x][y][z], shaderProgram, model);
	//         }
	//     }
	// }
	glDeleteProgram(shaderProgram);
}
