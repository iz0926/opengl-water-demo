# CS1750 Computer Graphics: Interactive OpenGL Water Demo

Interactive OpenGL/GLFW sandbox with dynamic water, HDR/bloom, stone skipping, fishing lure, boat driving, treasure chests, and spatial audio. Code is split into small modules (math, GL helpers, meshes, waves, stones, rod, chest, input, audio).

## Build

Dependencies: `glfw`, `glew`, `SDL2`, `SDL2_mixer`, and an OpenGL 3.3+ capable GPU. Dear ImGui is vendored in `imgui/`. On macOS with Homebrew the Makefile looks in `/opt/homebrew` by default. Install with:

```bash
brew install glfw glew sdl2 sdl2_mixer
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
- Audio: under `assets/audio`

## References
\begin{itemize}
  \item Shallow Water Equations: \url{https://en.wikipedia.org/wiki/Shallow_water_equations}
  \item J. Tessendorf, \textit{Effective Water Simulation from Physical Models}, GPU Gems, NVIDIA: \url{https://developer.nvidia.com/gpugems/gpugems/part-i-natural-effects/chapter-1-effective-water-simulation-physical-models}
  \item Dotcrossdot, \textit{Water Ocean Shader}: \url{https://medium.com/dotcrossdot/water-ocean-shader-9173e0977f98}
  \item I. A. Strumberger et al., \textit{Real-Time Water Simulation}, NAUN: \url{https://www.naun.org/main/NAUN/computers/ijcomputers-41.pdf}
  \item Derivation of skipping stones over water: \url{https://physics.stackexchange.com/questions/176108/deriving-equation-for-skipping-stones-over-water}
  \item Physics of Floating Cube Spring Movement: \url{https://www.myphysicslab.com/springs/single-spring-en.html} and \url{https://gafferongames.com/post/spring_physics/}
\end{itemize}

\subsection*{3D Models}
\begin{itemize}
  \item Fish model: \url{https://free3d.com/3d-model/fish-v1--996288.html}
  \item Speedboat model: \url{https://free3d.com/3d-model/speedboat-v01--840133.html}
  \item Treasure chest model: \url{https://free3d.com/3d-model/treasure-chest-91359.html}
\end{itemize}

\subsection*{Code Libraries and Tools}
\begin{itemize}
  \item jrafix post-processing effects: \url{https://gitlab.com/kernitus/jrafix}
  \item Dear ImGui: \url{https://github.com/ocornut/imgui}
\end{itemize}

\subsection*{Audio Assets}
\begin{itemize}
  \item water\_drop1 / water\_drop2: \url{https://www.youtube.com/watch?v=6L0HBSu_ZaY}
  \item water\_drop3: \url{https://pixabay.com/sound-effects/water-drop-stereo-257180/}
  \item relaxing zelda music + ocean waves: \url{https://www.youtube.com/watch?v=LfdCMBCt2r4}
  \item Speed Boat Ride Ambience: \url{https://www.youtube.com/watch?v=1frh0wqXb1M}
  \item Magic WHOOSH: \url{https://www.youtube.com/watch?v=pyzUhkKf2Qo}
  \item Magical Twinkle Sound Effect (HD): \url{https://www.youtube.com/watch?v=UZcSVrHi3-I}
  \item fish\_reel: \url{https://www.youtube.com/watch?v=HStwgFswd18}
  \item hit\_thud: \url{https://pixabay.com/sound-effects/cinematic-thud-fx-379991/}
  \item mouse\_click: \url{https://pixabay.com/sound-effects/mouse-click-290204/}
  \item game\_menu: \url{https://www.youtube.com/watch?v=sW8TKZtoND8}
  \item tiny\_splash: \url{https://pixabay.com/sound-effects/tiny-splash-83778/}
  \item underwater\_sounds: \url{https://www.youtube.com/watch?v=Pf4ZVHyA7Bg}
  \item success: \url{https://www.youtube.com/watch?v=n4qnDaSidJs}
\end{itemize}
