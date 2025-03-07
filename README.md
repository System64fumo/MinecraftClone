# MinecraftClone
Yet another minecraft clone.<br>
This project serves as a playground for me to goof around with OpenGL and 3D graphics.<br>
I mainly want to focus on optimizations here, So gameplay is not a priority.<br>

# Features
* First person camera control
* Keyboard movement
* "Infinite" world size
* Procedural world generation
* Block Highlights
* FPS Counter & Several profiling utils
* Vignette effect
* GUI

# Optimizations
* Render distance (Only keep track of chunks that matter not the whole world)
* Single large VBO
* Greedy meshing
* Multithreading (Terrain generates on another thread, So does meshing)
* DDA raycasting
* Basic face culling
* 3D Frustum culling

# Note
At the time of writing my main PC is an Orange Pi 5 Plus, So code may cater to it.<br>
That also means i will be targetting the ARM architecture.<br>
This project uses original Minecraft Assets, So in order to avoid copyright issues, This repo will provide simplified versions of the assets.<br>
This project is not affiliated with Mojang or Microsoft in any way.<br>

# Dependencies
* [GLFW](https://github.com/glfw/glfw)
* [GLEW](https://github.com/nigels-com/glew)
* [libpng](https://github.com/glennrp/libpng)
* OpenGL 3.0 ES (or newer)
* UPX (optional)

# Credits
Thanks to [Sean Barrett](https://github.com/nothings/stb/blob/master/stb_perlin.h) for the perlin noise generator.<br>
Thanks to [SourceGraph](https://sourcegraph.com/) for cody. (like 90% of this codebase is written by it lol)<br>
And obviously many thanks to [Notch](https://x.com/notch) for creating this childhood classic.<br>
