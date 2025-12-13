# CS1750 Project

Interactive OpenGL/GLFW demo with dynamic water, reflections, HDR/bloom, and a stone-skipping simulation. Code is split into small modules (math, GL helpers, meshes, waves, stones, input).

## Build

Dependencies: `glfw`, `glew`, and an OpenGL 3.3+ capable GPU. On macOS with Homebrew the Makefile looks in `/opt/homebrew` by default.

```bash
make clean && make
```

## Run

```bash
./cs1750_project
```

Controls:
- `W/A/S/D` move on XZ, `Q/E` down/up
- Arrow keys yaw/pitch (or enable mouse look)
- `F` hold to charge a throw, release to skip a stone (spawns ripples, can bump cubes)
- `V` press to switch between Cube 1 and Cube 2 so that the selected cube responds to the controls below
- Left mouse button (on selected cube) press and hold to push cube under water, releasing mouse button causes selected cube to pop back up and bob on the water surface
- `Esc` quits

## Features
- HDR + bloom + tone mapping (bright-pass → separable blur → composite).
- Planar reflection/refraction for water via offscreen FBOs and clip planes.
- Gerstner waves + normal/DuDv maps, depth-aware refraction, foam, tiled water mesh for lake that looks infinite.
- Stone impacts spawn ripples; ripple field nudges floating cubes.
- Interactive, buoyant cubes that can be pushed under water and released to float/bounce.
- Directional shadows, fog/underwater mode, caustics on scene geometry.
- Modular helpers: `Math.*`, `GLHelpers.*`, `Mesh.*`, `Waves.*`, `Stone.*`, `Input.*`; render passes live in `main.cpp`.
