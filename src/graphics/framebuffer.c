#include "main.h"
#include "gui.h"

// Framebuffer objects
unsigned int FBO, colorTexture, RBO;
unsigned int quadVAO, quadVBO;

char block_face;
int world_block_x;
int world_block_y;
int world_block_z;

void setup_framebuffer(int width, int height) {
	// Delete existing resources if they exist
	if (colorTexture)
		glDeleteTextures(1, &colorTexture);
	if (RBO)
		glDeleteRenderbuffers(1, &RBO);

	// Create or recreate framebuffer if it doesn't exist
	if (!FBO)
		glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	// Create/recreate color attachment texture
	glGenTextures(1, &colorTexture);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);

	// Create/recreate depth/stencil renderbuffer
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void init_fullscreen_quad() {
	float quadVertices[] = {
		// positions   // texCoords
		-1.0f,  1.0f,  0.0f, 1.0f, // Top-left
		-1.0f, -1.0f,  0.0f, 0.0f, // Bottom-left
		 1.0f,  1.0f,  1.0f, 1.0f, // Top-right
		 1.0f, -1.0f,  1.0f, 0.0f  // Bottom-right
	};

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindVertexArray(quadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void render_to_framebuffer() {
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glClearColor(0.471f * sky_brightness, 0.655f * sky_brightness, 1.0f * sky_brightness, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, block_textures);
	glUniform1i(glGetUniformLocation(shaderProgram, "textureAtlas"), 0);

	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER, true);
	#endif

	setup_matrices();
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, view);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, projection);

	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	vec3 pos;
	pos.x = global_entities[0].x;
	pos.y = global_entities[0].y + global_entities[0].eye_level;
	pos.z = global_entities[0].z;

	render_chunks();

	get_targeted_block(pos, dir, 5.0f, &world_block_x, &world_block_y, &world_block_z, &block_face);
	if (block_face != 'N')
		draw_block_highlight(world_block_x + 1, world_block_y + 1, world_block_z + 1);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER, true);
	#endif
}

void render_to_screen() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glUseProgram(postProcessingShader);
	glBindVertexArray(quadVAO);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);

	#ifdef DEBUG
	profiler_start(PROFILER_ID_UI, true);
	#endif
	render_ui();
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_UI, true);
	#endif
}

void cleanup_framebuffer() {
	glDeleteFramebuffers(1, &FBO);
	glDeleteTextures(1, &colorTexture);
	glDeleteRenderbuffers(1, &RBO);
}
