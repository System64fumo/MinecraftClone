#version 300 es
precision highp float;
precision highp int;
precision highp sampler2D;

in vec3 TexCoords;
out vec4 FragColor;

uniform float time;

vec3 get_sky_color(vec3 direction, float t) {
	vec3 dir = normalize(direction);

	float vertical_pos = dir.y;
	float sun_angle = t * 2.0 * 3.14159;
	float sun_height = sin(sun_angle);
	
	vec3 day_top = vec3(0.502, 0.659, 1.0);
	vec3 day_bottom = vec3(0.753, 0.847, 1.0);
	
	vec3 sunset_top = vec3(0.761,0.322,0.224);
	vec3 sunset_bottom = vec3(0.471,0.329,0.349);
	
	vec3 night_top = vec3(0.004,0.004,0.008);
	vec3 night_bottom = vec3(0.035,0.043,0.075);
	
	vec3 top_color, bottom_color;
	// Full day
	if (sun_height > 0.7) {
		top_color = day_top;
		bottom_color = day_bottom;
	}

	// Day to sunset transition
	else if (sun_height > 0.0) {
		float blend = sun_height / 0.7;
		top_color = mix(sunset_top, day_top, blend);
		bottom_color = mix(sunset_bottom, day_bottom, blend);
	}

	// Sunset to night transition
	else if (sun_height > -0.3) {
		float blend = (sun_height + 0.3) / 0.3;
		top_color = mix(night_top, sunset_top, blend);
		bottom_color = mix(night_bottom, sunset_bottom, blend);
	}

	// Full night
	else {
		top_color = night_top;
		bottom_color = night_bottom;
	}
	

	float horizon_rotation = sun_angle * 0.1;
	float rotated_vertical_pos = vertical_pos + sin(horizon_rotation) * 0.1;

	float transition_width = 0.08;
	float horizon_center = 0.0;
	
	float blend_factor;
	
	if (rotated_vertical_pos < horizon_center - transition_width) {
		blend_factor = 1.0;
	} else if (rotated_vertical_pos > horizon_center + transition_width) {
		blend_factor = 0.0;
	} else {
		float transition_pos = (rotated_vertical_pos - horizon_center) / transition_width;
		blend_factor = 1.0 - smoothstep(-1.0, 1.0, transition_pos);
	}

	vec3 sky_color = mix(top_color, bottom_color, blend_factor);

	float horizontal_variation = sin(dir.x * 3.14159) * cos(dir.z * 3.14159) * 0.02;
	sky_color += horizontal_variation;

	if (sun_height > -0.3 && sun_height < 0.3) {
		float glow_intensity = 1.0 - abs(sun_height) / 0.3;
		float horizon_distance = abs(rotated_vertical_pos - horizon_center);
		float glow = exp(-horizon_distance * 8.0) * glow_intensity * 0.3;
		sky_color += vec3(glow * 1.0, glow * 0.6, glow * 0.2);  // Warm glow
	}

	return sky_color;
}

void main() {
	vec3 sky_color = get_sky_color(TexCoords, time);
	FragColor = vec4(sky_color, 1.0);
}