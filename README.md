# DirectX 11 Renderer
This renderer is built with DirectX 11 API. Several rendering techniques and effects were implemented. Here is a list of them:
- indexed and instanced \[mesh\] rendering
- reversed depth (for increased precision)
- batching rendering by shaders, textures, uniforms...
- hologram effect created with geometry shader
- dissolution/spawning effect using Alpha-to-Coverage
- tesselation
- skybox
- PBR (Lambertian diffuse, Cook-Torrance specular - GGX for D, height-correlated GGX Smith for D, Shlick's approx. for F)
- IBL
- shadow mapping (for directional, point and spot lights) with shadow acne suppressing methods
- CPU Textures with 6-way lightmapping
- deferred shading
- decals
- GPU particles (with screen-space collisions) using compute shaders
- Anti-Aliasing (MSAA, FXAA)
- fog and volumetric fog
- a few post-process effects: EV100 Exposure Value, ACES tone mapping, gamma correction, bloom

Here are a few images:
<p> General rendering, skybox, CPU particle system <br>
<img src="https://github.com/OftenDeadKanji/DirectX11-Renderer/assets/32665400/bcda0c48-6927-4d25-8307-301b3408d3b0" width=50%>
</p>

<p> Hologram effect <br>
<img src="https://github.com/OftenDeadKanji/DirectX11-Renderer/assets/32665400/3db1172e-4dc6-4865-8170-9649af9bcd06" width=50%>
</p>
    
<p> Decals <br>
<img src="https://github.com/OftenDeadKanji/DirectX11-Renderer/assets/32665400/983f807a-a373-4c57-8d6f-289550468d06" width=50%>
</p>

<p> Fog <br>
<img src="https://github.com/OftenDeadKanji/DirectX11-Renderer/assets/32665400/edbf50c2-e948-47df-8ab1-5f1c3d1d973f" width=50%>
</p>

<p> Volumetric fog <br>
<img src="https://github.com/OftenDeadKanji/DirectX11-Renderer/assets/32665400/ef1f42ac-96b0-44d7-9a00-a4ff189d0b4d" width=50%>
</p>
