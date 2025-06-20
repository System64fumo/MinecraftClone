# MinecraftClone
Yet another minecraft clone.<br>
This project serves as a playground for me to goof around with OpenGL and 3D graphics.<br>
I mainly want to focus on optimizations here, So gameplay is not a priority.<br>

# Features
* First person camera control
* Keyboard movement
* "Infinite" world size
* Procedural world generation
* Ordered chunk loading
* Block Highlights
* FPS Counter & Several profiling utils
* Vignette effect
* GUI
* Lighting (Experimental)

# Optimizations
* Render distance (Only keep track of chunks that matter not the whole world)
* Batch rendering
* Greedy meshing
* DDA raycasting
* AABB collision
* Basic back face culling
* 3D Frustum culling
* Occlusion culling (Currently disabled)
* Data packing

# Notes
This project has been tested on ARM64 only, It should still work on x86_64 though. (In theory)<br>
This project aims to use original Minecraft Assets, So in order to avoid copyright issues i've provided simplified versions of the assets.<br>
This project is not affiliated with Mojang or Microsoft in any way.<br>

# Dependencies
* [GLFW](https://github.com/glfw/glfw)
* [GLEW](https://github.com/nigels-com/glew)
* [libwebp](https://chromium.googlesource.com/webm/libwebp)
* OpenGL 3.0 ES (or newer)
* UPX (optional)

# Credits
Thanks to [Sean Barrett](https://github.com/nothings/stb/blob/master/stb_perlin.h) for the perlin noise generator.<br>
Thanks to [SourceGraph](https://sourcegraph.com/) for cody.<br>
And obviously many thanks to [Notch](https://x.com/notch) for creating this childhood classic.<br>
