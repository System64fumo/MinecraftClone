# MinecraftClone
Yet another minecraft clone.<br>
This project serves as a playground for me to goof around with OpenGL and 3D graphics.<br>

I'm not really a game dev nor do i care about becoming one,<br>
Just thought this is interesting and wanted to give it a try.<br>

I won't focus on features or game mechanics here, I mainly want to learn how 3D stuff works.<br>

# Features
* First person camera control
* Keyboard movement
* "Infinite" world size (Currently limited but can be very easily increased without a performance hit)
* Procedural world generation
* Block Highlights
* FPS Counter & Several profiling utils
* Minimap (Debug builds only atm)

# Optimizations
* Render distance (Only keep track of chunks that matter not the whole world)
* VAO and VBO
* Batching
* Greedy meshing
* Face culling

# Note
At the time of writing my main PC is an Orange Pi 5 Plus, So code may cater to it.<br>
That also means i will be targetting the ARM architecture.<br>

# Credits
Thanks to [Sean Barrett](https://github.com/nothings/stb/blob/master/stb_perlin.h) for the perlin noise generator.<br>
Thanks to [SourceGraph](https://sourcegraph.com/) for cody. (like 90% of this codebase is written by it lol)<br>
And obviously many thanks to [Notch](https://x.com/notch) for creating this childhood classic.<br>
