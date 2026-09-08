# Building & Running Vajra

## 1. Install dependencies

**Ubuntu / Kubuntu / Debian**
```bash
sudo apt update
sudo apt install build-essential cmake ninja-build git \
                 libsdl2-dev libgl1-mesa-dev
```

**Arch / Manjaro**
```bash
sudo pacman -S base-devel cmake ninja git sdl2 mesa
```

**Fedora**
```bash
sudo dnf install gcc-c++ cmake ninja-build git SDL2-devel mesa-libGL-devel
```

GLM, EnTT and nlohmann/json are downloaded automatically by CMake on the first
configure, so that step needs an internet connection (about 40 seconds).
GLAD is already vendored in `ThirdParty/glad` — nothing to generate.

## 2. Run it in CLion

1. **File → Open** and select the `Vajra` folder. Do not open a single file —
   open the folder, so CLion sees `CMakeLists.txt`.
2. CLion detects `CMakePresets.json` and offers **Debug** and **Release**
   profiles. Enable both; pick **Release** to actually play.
3. Wait for "CMake project is loaded" in the status bar. The first load
   downloads dependencies.
4. In the top-right run configuration dropdown, choose **Vajra**.
5. Press the green **Run** button (or `Shift+F10`).

That is the whole setup. There is no working-directory to configure: the asset
path is compiled in as an absolute path, so the game finds its shaders and
levels no matter where CLion launches it from.

**If the Run dropdown is empty**, click *Reload CMake Project*
(the circular arrow in the CMake tool window).

**If you get sanitizer warnings while debugging**, that is the Debug profile
doing its job. Switch to Release to play.

## 3. Or build from the command line

```bash
cmake --preset release
cmake --build --preset release -j$(nproc)
./build/vajra
```

Run the automated checks:
```bash
ctest --preset release
```
This validates all three missions offline — that objectives are reachable, no
guard is spawned inside a wall, and every patrol route is walkable. It needs no
display, so it works in CI.

## 4. Install or package

```bash
./packaging/install.sh ~/.local     # user install, no sudo
sudo ./packaging/install.sh         # system-wide to /usr/local
./packaging/build_appimage.sh       # portable AppImage
```

A Flatpak manifest is at `packaging/io.github.vajra.Vajra.yml`.

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `SDL_CreateWindow failed ... OpenGL not available` | No GPU/display. You are on a headless machine or in a container without GPU passthrough. |
| `Could NOT find SDL2` | Install `libsdl2-dev` (see step 1). |
| Black screen, no errors | Press `F7`/`F8`/`F10` to disable SSAO, bloom, FXAA. Some old drivers dislike float framebuffers. |
| GLM `experimental extension` error | Your build is missing `GLM_ENABLE_EXPERIMENTAL`. Delete `build/` and reconfigure. |
| Low frame rate | Use the Release profile, not Debug. Debug enables sanitizers and is 5-10x slower. |
