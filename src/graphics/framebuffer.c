#include "main.h"
#include "gui.h"
#include "config.h"

// Framebuffer objects
unsigned int FBO, colorTexture, RBO;
unsigned int quadVAO, quadVBO;
uint8_t last_ui_state = 0;

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
	draw_calls = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glClearColor(0.471f * settings.sky_brightness, 0.655f * settings.sky_brightness, 1.0f * settings.sky_brightness, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(shaderProgram);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, block_textures);
	glUniform1i(atlas_uniform_location, 0);

	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER, true);
	#endif

	setup_matrices();
	glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, view);
	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, projection);

	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	render_chunks();

	char block_face = 'N';
	vec3 block_pos = {0};
	get_targeted_block(global_entities[0], dir, 5.0f, &block_pos, &block_face);
	if (block_face != 'N')
		draw_block_highlight(block_pos);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER, true);
	#endif
}

void render_to_screen() {
	glDisable(GL_DEPTH_TEST);
	glUseProgram(postProcessingShader);
	if (last_ui_state != ui_state) {
		glUniform1i(ui_state_uniform_location, ui_state);
		last_ui_state = ui_state;
	}

	glBindVertexArray(quadVAO);
	glBindTexture(GL_TEXTURE_2D, colorTexture);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

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
