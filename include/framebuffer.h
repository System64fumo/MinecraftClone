#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

extern unsigned int texture_fb_color, texture_fb_depth;

void setup_framebuffer(int width, int height);
void render_to_framebuffer();
void render_to_screen();
void cleanup_framebuffer();

#endif