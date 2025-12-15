# CS1750 Computer Graphics: Interactive OpenGL Water Demo

Interactive OpenGL/GLFW sandbox with dynamic water, HDR/bloom, stone skipping, fishing lure, boat driving, treasure chests, and spatial audio. Code is split into small modules (math, GL helpers, meshes, waves, stones, rod, chest, input, audio).

## Build

Dependencies: `glfw`, `glew`, `SDL2`, `SDL2_mixer`, and an OpenGL 3.3+ capable GPU. Dear ImGui is vendored in `imgui/`. Audio assets are stored via Git LFS.

On macOS with Homebrew the Makefile looks in `/opt/homebrew` by default. Install with:

```bash
brew install glfw glew sdl2 sdl2_mixer
brew install git-lfs
git lfs install
make clean && make
```

## Run

```bash
./cs1750_project
```

Controls:
- `W/A/S/D` move on XZ, `Q/E` down/up
- Arrow keys yaw/pitch; right-drag to look in free mode (slider in menu to tune sensitivity)
- `B` toggle boat mode (first-person from the bow; steering follows the hull)
- `F` hold to charge, release to skip a stone (ripples, can bump cubes/boat)
- `R` hold/release to throw the red cube lure (parabolic flight, single splash; only way to catch fish)
- `V` switch controlled cube (Cube 1 vs Cube 2)
- Left mouse: press/hold to push the selected cube under, release to pop it up
- `Esc` opens the control panel (resume/exit, right-drag sensitivity slider, BGM volume/mute, track skip)

Menu: ESC opens a top panel (ImGui) with control hints and sliders.

## Features
- HDR + bloom + tone mapping (bright-pass → separable blur → composite).
- Planar reflection/refraction for water via offscreen FBOs and clip planes.
- Gerstner waves + normal/DuDv maps, depth-aware refraction, foam, infinite tiled water mesh.
- Stone impacts spawn ripples; ripple field nudges floating cubes.
- Fishing red lure: charged throw with single splash; fish are only caught by the lure.
- Fish: wander/avoid boat+cubes, bank when turning, stick to lure briefly when caught.
- Interactive, buoyant cubes that can be pushed under and released to float/bounce; boat can push cubes.
- Treasure chest: spawns every ~5 real minutes with tall glow marker and despawns after 15 seconds; collect for prizes counter. 
- Dynamic day/night sun and sky gradient; time-of-day HUD.
- Directional shadows, fog/underwater mode, caustics on scene geometry.
- Audio: looping BGM, boat engine with speed-based volume, underwater ambience with ducked BGM, splashes/drops, chest spawn/pickup, fish catch, menu clicks, reel sound while charging `R`.
- In-game ImGui panel (ESC) with control reference, sensitivity slider, BGM controls (volume, mute, track skip), resume/exit.
- Modular helpers: `Math.*`, `GLHelpers.*`, `Mesh.*`, `Waves.*`, `Stone.*`, `Rod.*`, `Chest.*`, `Input.*`, `Audio.*`; render passes live in `main.cpp`.

## Assets
- Models: under `assets/models/SpeedBoat`, `assets/models/Fish`, `assets/models/chest.obj` (OBJ/MTL).
- Audio: under `assets/audio` (Git LFS). After cloning, run `git lfs install` and `git lfs pull` to download the WAVs; otherwise you’ll see “Unrecognized audio format” errors.

## References
- Shallow Water Equations: https://en.wikipedia.org/wiki/Shallow_water_equations  
- J. Tessendorf, *Effective Water Simulation from Physical Models*, GPU Gems (NVIDIA): https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models  
- Dotcrossdot, *Water Ocean Shader*: https://medium.com/dotcrossdot/water-ocean-shader-9173e0977f98  
- I. A. Strumberger et al., *Real-Time Water Simulation*, NAUN: https://www.naun.org/main/NAUN/computers/ijcomputers-41.pdf  
- Derivation of skipping stones over water: https://physics.stackexchange.com/questions/176108/deriving-equation-for-skipping-stones-over-water  
- Physics of Floating Cube Spring Movement: https://www.myphysicslab.com/springs/single-spring-en.html and https://gafferongames.com/post/spring_physics/

### 3D Models
- Fish model: https://free3d.com/3d-model/fish-v1--996288.html  
- Speedboat model: https://free3d.com/3d-model/speedboat-v01--840133.html  
- Treasure chest model: https://free3d.com/3d-model/treasure-chest-91359.html

### Code Libraries and Tools
- jrafix post-processing effects: https://gitlab.com/kernitus/jrafix  
- Dear ImGui: https://github.com/ocornut/imgui

### Audio Assets
- water_drop1 / water_drop2: https://www.youtube.com/watch?v=6L0HBSu_ZaY  
- water_drop3: https://pixabay.com/sound-effects/water-drop-stereo-257180/  
- relaxing zelda music + ocean waves: https://www.youtube.com/watch?v=LfdCMBCt2r4  
- Speed Boat Ride Ambience: https://www.youtube.com/watch?v=1frh0wqXb1M  
- Magic WHOOSH: https://www.youtube.com/watch?v=pyzUhkKf2Qo  
- Magical Twinkle Sound Effect (HD): https://www.youtube.com/watch?v=UZcSVrHi3-I  
- fish_reel: https://www.youtube.com/watch?v=HStwgFswd18  
- hit_thud: https://pixabay.com/sound-effects/cinematic-thud-fx-379991/  
- mouse_click: https://pixabay.com/sound-effects/mouse-click-290204/  
- game_menu: https://www.youtube.com/watch?v=sW8TKZtoND8  
- tiny_splash: https://pixabay.com/sound-effects/tiny-splash-83778/  
- underwater_sounds: https://www.youtube.com/watch?v=Pf4ZVHyA7Bg  
- success: https://www.youtube.com/watch?v=n4qnDaSidJs
