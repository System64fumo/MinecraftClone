#include "main.h"
#include "shaders.h"
#include "skybox.h"
#include "gui.h"
#include "textures.h"
#include "config.h"
#include <stdio.h>
#include <string.h>

// Framebuffer objects
unsigned int FBO, RBO;
unsigned int quadVAO, quadVBO;
uint8_t last_ui_state = 0;
unsigned int texture_fb_color, texture_fb_depth;
GLuint depth_loc = 0;

void setup_framebuffer(int width, int height) {
	if (!FBO) glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	if (!texture_fb_color || glIsTexture(texture_fb_color) == GL_FALSE) {
		if (texture_fb_color) glDeleteTextures(1, &texture_fb_color);
		glGenTextures(1, &texture_fb_color);
	}
	glBindTexture(GL_TEXTURE_2D, texture_fb_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_fb_color, 0);

	// Depth texture
	if (!texture_fb_depth || glIsTexture(texture_fb_depth) == GL_FALSE) {
		if (texture_fb_depth) glDeleteTextures(1, &texture_fb_depth);
		glGenTextures(1, &texture_fb_depth);
	}
	glBindTexture(GL_TEXTURE_2D, texture_fb_depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture_fb_depth, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("Framebuffer not complete!\n");
	}

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
	#ifdef DEBUG
	profiler_start(PROFILER_ID_FRAMEBUFFER, true);
	#endif
	draw_calls = 0;
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	
	// Clear all buffers
	glClear(GL_DEPTH_BUFFER_BIT);
	setup_matrices();

	// Draw skybox
	skybox_render();

	glUseProgram(world_shader);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, block_textures);

	#ifdef DEBUG
	profiler_stop(PROFILER_ID_FRAMEBUFFER, true);
	#endif

	#ifdef DEBUG
	profiler_start(PROFILER_ID_RENDER, true);
	#endif

	matrix4_identity(model);
	glUniformMatrix4fv(model_uniform_location, 1, GL_FALSE, model);
	glUniformMatrix4fv(view_uniform_location, 1, GL_FALSE, view);
	glUniformMatrix4fv(projection_uniform_location, 1, GL_FALSE, projection);

	glEnable(GL_DEPTH_TEST);
	render_chunks();

	char block_face = 'N';
	vec3 block_pos = {0};
	vec3 dir = get_direction(global_entities[0].pitch, global_entities[0].yaw);
	uint8_t block_id;
	get_targeted_block(global_entities[0], dir, 5.0f, &block_pos, &block_face, &block_id);
	if (block_face != 'N')
		draw_block_highlight(block_pos, block_id);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	#ifdef DEBUG
	profiler_stop(PROFILER_ID_RENDER, true);
	#endif
}

void render_to_screen() {
	glDisable(GL_DEPTH_TEST);
	glUseProgram(post_process_shader);

	float inv_projection[16];
	float inv_view[16];
	matrix4_inverse(projection, inv_projection);
	matrix4_inverse(view, inv_view);

	// Bind all textures
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture_fb_color);

	glUniformMatrix4fv(inv_projection_uniform_location, 1, GL_FALSE, inv_projection);
	glUniformMatrix4fv(inv_view_uniform_location, 1, GL_FALSE, inv_view);
	glUniform1f(far_uniform_location, far);

	if (last_ui_state != ui_state) {
		glUniform1i(ui_state_uniform_location, ui_state);
		last_ui_state = ui_state;
	}

	glBindVertexArray(quadVAO);
	glActiveTexture(GL_TEXTURE0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	draw_calls++;

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
	glDeleteTextures(1, &texture_fb_color);
	glDeleteTextures(1, &texture_fb_depth);
	glDeleteRenderbuffers(1, &RBO);
}