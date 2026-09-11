# NoesisGUI Integration Sample

[**NoesisGUI**](https://www.noesisengine.com/) is a lightweight, cross-platform XAML UI engine that renders
WPF/XAML interfaces (styles, data binding, animations, vector graphics) on top of a game engine's renderer.
This sample integrates NoesisGUI with UNIGINE: it implements a custom Noesis render device on top of the
UNIGINE rendering API and shows XAML both as screen-space overlays and as a GUI mapped onto a 3D surface in
the world, wired to the engine through a data context (sun-angle and time-of-day controls).

## How to Run the Sample

### Prerequisites

- [**UNIGINE SDK Browser**](https://developer.unigine.com/en/docs/latest/start/installing_sdk) (latest version)
- **UNIGINE SDK Community** or **Engineering** edition (**Sim** upgrade supported)
- **Visual Studio 2022** (Windows) or GCC/Clang (Linux)

### Third-party dependency

This sample uses the **NoesisGUI SDK** version **3.2.13** ([downloads](https://www.noesisengine.com/developers/downloads.php)).
The SDK is **not** bundled in this repository. It is resolved by `source/cmake/modules/FindNoesisGUI.cmake` from,
in order:

1. an in-tree drop at `source/NoesisSDK/` (git-ignored), or
2. `$UNIGINE_3RDPARTY_DIR/NoesisGUI/Cpp/3.2.13/`, if that environment variable points to a directory where the SDK
   is kept outside the project.

Either location must contain the standard SDK layout: `Include/` and `Bin/<windows_x86_64|linux_x86_64>/`,
plus `Lib/windows_x86_64/` on Windows (the import library is not used on Linux). The build copies the
NoesisGUI runtime (`Noesis.dll` / `libNoesis.so`) next to the application in `bin/`. The vendored NoesisGUI
"App framework" interactivity sources ship in `source/Src/Packages/App/Interactivity/` and are compiled into
the sample.

### Rendering backends

- **Windows:** Direct3D 12 or Vulkan. The Direct3D 12 backend loads the D3D12 Agility SDK redistributable from
  `bin/D3D12/` (relative to the executable).
- **Linux:** Vulkan.

### Fonts

The sample installs its own Noesis font provider, so the **only** fonts available are the ones shipped in
`data/ui/` — no system font is reachable. Every face is registered from its own folder, and a `FontFamily`
resolves only when it addresses the family through that folder: relative to the referencing XAML
(`Fonts/#PT Root UI` inside `data/ui/themes/noesis/`), or relative to the data root when the value has no
XAML context, i.e. comes from a binding or from the font fallback list (`ui/fonts/#Muli`).

`main.cpp` therefore points the fallback chain at a bundled face. Naming a system family such as `Arial`
there works on Windows only — Noesis resolves it through DirectWrite — and renders every character as a
`.notdef` box on Linux.

### Step-by-Step Guide

1. **Clone or download** the sample.
2. **Open SDK Browser** and make sure you have the latest version.
3. **Add the sample project**: *My Projects* → *Add Existing* → select the `.project` file that matches your OS
   (`*_win_*` / `*_lin_*`), edition, and precision → *Import Project*.
4. **Repair** the project (only essential files are in Git; SDK Browser restores the rest), then *Configure Project*.
5. **Open** the project in your IDE: load the folder containing `source/CMakeLists.txt` (Visual Studio 2022 recommended).
6. **Build** and **Run**.

> [!WARNING]
> Precision must match the `.project` you selected. The coordinate precision is set in `source/CMakeLists.txt`:
> ```diff
> - set(UNIGINE_DOUBLE False CACHE BOOL "Double coords")
> + set(UNIGINE_DOUBLE True  CACHE BOOL "Double coords")
> ```
> Use a `*_double.project` for double-precision builds and a `*_float.project` for float.

## What the Sample Contains

```
unigine-noesis-cpp-integration-sample/
  source/     — main.cpp (SystemLogic/WorldLogic), Noesis integration glue
                (NoesisIntegration, NoesisView, NoesisRenderDevice, NoesisTexture,
                 NoesisProviders, NoesisShader, NoesisDataContext, ObjectNoesisGui),
                Src/Packages/App/Interactivity/ (vendored NoesisGUI App framework),
                CMakeLists.txt + cmake/ (Engine + NoesisGUI resolution)
  data/       — noesis_sample.world, noesis/ (materials + shaders), ui/ (XAML, fonts, themes),
                root_mount.umount
  README.md   — this file
  *.project   — SDK Browser project files (per platform / edition / precision)
```

## If the Sample Fails to Run

- Re-check every setup step above.
- Ensure the **NoesisGUI SDK 3.2.13** is reachable (in-tree `source/NoesisSDK/` or `$UNIGINE_3RDPARTY_DIR/NoesisGUI/Cpp/3.2.13/`),
  and that `Noesis.dll` / `libNoesis.so` ended up in `bin/` next to the executable.
- On Windows with the Direct3D 12 backend, verify the D3D12 Agility SDK redistributable is present in `bin/D3D12/`.
- Ensure `UNIGINE_DOUBLE` matches the current build type (double/float) and the chosen `.project`.
- Use the `.project` file for your platform and SDK edition.
- Verify your SDK version is not older than the project's specified version.
- C++/CMake issues in Visual Studio: right-click the project → **Delete Cache and Reconfigure**, then rebuild.
