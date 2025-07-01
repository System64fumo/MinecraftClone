extern unsigned int world_shader, post_process_shader, ui_shader, skybox_shader, clouds_shader;

extern unsigned int model_uniform_location;
extern unsigned int atlas_uniform_location;
extern unsigned int view_uniform_location;
extern unsigned int projection_uniform_location;
extern unsigned int ui_projection_uniform_location;
extern unsigned int ui_state_uniform_location;
extern unsigned int screen_texture_uniform_location;
extern unsigned int texture_fb_depth_uniform_location;
extern unsigned int near_uniform_location;
extern unsigned int far_uniform_location;
extern unsigned int clouds_offset_uniform_location;

void load_shaders();
void load_shader_constants();
void cache_uniform_locations();