# Ghost Kitty / OpenGL Starter (OBJ + MTL + textures)

## Fixed
- Scene is split by `usemtl` and rendered with its respective diffuse texture (`map_Kd`), material color (`Kd`) and opacity map (`map_d`).
- Models/modelCG1.mtl now references `texture/<filename>` instead of absolute Windows paths.
- The bundled images are stored at `Models/texture/` (12 files). Old `Textures/` contains tutorial assets, no need to duplicate.
- Hidden the unintended default object `o Cube` from OBJ during loading; the original OBJ is unchanged.
- Automatic camera framing uses scene bounding box; arrow keys orbit, W/S zoom, Esc exit.
- Disabled face culling for decorative flat surfaces.

## Build in VS Code
1. Open `OpenGLStarter` folder, containing CMakeLists.txt.
2. `CMake: Delete Cache and Reconfigure` if the folder was moved.
3. Configure the installed Visual Studio 2022 x64 Kit and build.
4. Run `OpenGLStarter`.

## Important limitations
- OBJ/MTL cannot reproduce Blender procedural shaders, color management, lighting or transparency exactly. The new renderer supports diffuse/opacity textures and alpha cutouts, but not full Blender PBR.
- For a cleaner Blender export, remove/disable the unused default `Cube` before exporting.
- The original MTL referred to `rolll_RollCake_Op.png`, which was absent from the ZIP. That unavailable reference was removed; diffuse PNG alpha is still applied.
- This project is updated but was not compiled or visually executed on Windows in this environment.
