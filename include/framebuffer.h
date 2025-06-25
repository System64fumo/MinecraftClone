#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

extern unsigned int texture_fb_color, texture_fb_depth, accum_texture, reveal_texture;

void setup_framebuffer(int width, int height);
void init_fullscreen_quad();
void render_to_framebuffer();
void render_to_screen();
void cleanup_framebuffer();

#endif