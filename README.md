# MinecraftClone
This project serves as a playground for me to learn OpenGL and Graphics Programming.<br>
I mainly want to explore different optimization methods here, So gameplay/features won't be a priority.<br>

![preview](https://github.com/System64fumo/MinecraftClone/blob/main/preview.png "preview")

<table>
  <thead>
	<tr>
	  <th width="500px"> Features</th>
	  <th width="500px">Optimizations</th>
	</tr>
  </thead>
  <tbody>
  <tr width="600px">
	  <td>

* First person camera control<br>
* "Infinite" procedural world generation<br>
* Cave generation<br>
* Structures (Trees)<br>
* Block Highlights<br>
* FPS Counter<br>
* Vignette effect<br>
* Fog<br>
* GUI<br>
* 3D Clouds<br>

</td>
<td>

* Render distance<br>
* Batch rendering<br>
* Greedy meshing<br>
* DDA raycasting<br>
* AABB collision<br>
* Basic back face culling<br>
* Frustum culling<br>
* Directional culling<br>
* Data packing<br>

</td>
</tr>

  </tbody>
</table>



<table>
  <thead>
	<tr>
	  <th width="500px">Recommended requirements</th>
	  <th width="500px">Minimum requirements</th>
	</tr>
  </thead>
  <tbody>
  <tr width="600px">
	  <td>

* Quad core 1.8GHz CPU<br>
* Quad core 600Mhz GPU capable of GLES 3.0<br>
* 512mb of RAM<br>

</td>
<td>

* Single core 1.0GHz CPU<br>
* Single core 800Mhz GPU capable of GLES 3.0 (Optional)<br>
* 256mb of RAM<br>

</td>
</tr>

  </tbody>
</table>


> [!IMPORTANT]  
> For very obvious reasons i cannot re-distribute original Minecraft assets, Simplified alternatives are provided as placeholders..<br>
> This project is not affiliated with Microsoft or Mojang Studios in any way, All rights belong to their respective holders.<br>


# Controls
WASD: Movement<br>
Space: Jump<br>
Ctrl: Sprint<br>
R: Reload shaders<br>

# Config
The game stores it's config in ~/.config/ccraft/config.ini<br>
Name is subject to change but the config contains very basic stuff like:<br>
Initial window size, fov, render distance, culling or fancy settings<br>

# Dependencies
* [GLFW](https://github.com/glfw/glfw)
* [GLEW](https://github.com/nigels-com/glew)
* [libwebp](https://chromium.googlesource.com/webm/libwebp)
* UPX (optional)

# Credits
Thanks to [Sean Barrett](https://github.com/nothings/stb/blob/master/stb_perlin.h) for the perlin noise generator.<br>
Thanks to [DeepSeek](https://www.deepseek.com/en) and [Claude](https://claude.ai/) for assisting with development.<br>
And obviously many thanks to [Notch](https://x.com/notch) and the Mojang team for creating this classic video game.<br>
