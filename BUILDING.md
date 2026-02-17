# Building the Project

Building requires [CMake](https://cmake.org) v3.25 or newer. All other dependencies will be satisfied as part of the configure process, which requires an active Internet connection.

The project officially supports x64 and arm64 architectures for: Linux, macOS 11 or above, and Windows 10 or above. Other platforms may also work, but only those listed have been tested.

## Using Presets

Using CMake presets is the easiest way to configure and build the project. Presets require [Ninja](https://ninja-build.org).

### Available Presets

| Preset Name | Description             |
| :---------- | :---------------------- |
| `fs2`       | FreeSpace 2             |
| `fs2demo`   | FreeSpace 2 Demo        |
| `fs1`       | Descent: FreeSpace      |
| `fs1demo`   | Descent: FreeSpace Demo |

> [!TIP]
> When using a demo preset configure with `-D DEMO_GAME_DATA=<path_to_demo_files>` to allow creating a self-contained, ready to play package.

### Build types

Specify a build type to create release or debug binaries. If not specified, the build type is `RelWithDebInfo`.

| Build Type       | Description                                           |
| :--------------- | :---------------------------------------------------- |
| `RelWithDebInfo` | Release build with debug info ***(Default)***         |
| `Release`        | Release build stripped of debug info                  |
| `Debug`          | Debug build with logging and and extra error handling |

### Configure

```bash
cmake --preset <preset>
```

### Build

```bash
cmake --build --preset <preset> --config <build-type>
```

### Build the tool set (*optional*)

```bash
cmake --build --preset <preset>  --target toolset --config <build-type>
```

### Create bundle/installer

```bash
cmake --install build/presets/<preset> --config <build-type>
```

## Using an IDE

Use CMake to generate a project file for your IDE (see [cmake-generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html) documentation for more information).

As an example, to configure the ***Descent: FreeSpace*** demo version for Xcode on macOS:
```bash
cd build
cmake .. -G Xcode -D FS1=ON -D DEMO=ON
open freespace2.xcodeproj
```

### Build Flags

You can use the following configure options with CMake's `-D` flag to control the build

| Option           | Default Value | Description                              |
| :--------------- | :------------ | :--------------------------------------- |
| `FS1`            | OFF           | Build ***Descent: FreeSpace***           |
| `DEMO`           | OFF           | Build the demo version                   |
| `DEMO_GAME_DATA` | *unset*       | Path to demo data for use when packaging |

## Platform Specific Notes

### Linux

Using your package manager install the basic requirements, `cmake` and `ninja-build`. SDL has additional development packages which should be installed as well. Please see [README-linux](https://wiki.libsdl.org/SDL3/README-linux) from the SDL wiki for the needed dependencies.

> [!TIP]
> To aid in building on Linux a Dockerfile is provided with the project (under `dist/docker/`) which includes all of the dependencies required. Please refer to the [README](dist/docker/README.md) for additional information.

The `cmake --install ...` step will create an AppImage in `build/presets/<preset>/install/`. The `appimagetool` utility will be downloaded automatically if a system version wasn't found.

If you built the tool set as well then those files will also be included in the AppImage. Creating a symlink with the name of the tool pointing to the AppImage will allow easy use of each tool (e.g., `ln -s <game>.AppImage cfileutil`, then run `./cfileutil` to use the `.vp` archive utility).

### macOS

By default a build will be created for your current architecture (Intel or Apple Silicon). To use an alternate architecture add `-D CMAKE_OSX_ARCHITECTURES="x86_64"` (to build for Intel), or `-D CMAKE_OSX_ARCHITECTURES="arm64"` (to build for Apple Silicon) to the configure step.

> [!IMPORTANT]
> It is not currently possible to create a universal binary by specifying both architectures at the same time as dependencies must be configured for one architecture or the other. To create a universal binary you must compile and install the package for each architecture separately and then manually combine them into a univeral package using `lipo`.

The `cmake --install ...` step will create an app bundle in `build/presets/<preset>/install/`. The app bundle will be signed with an ad-hoc signature.

If you built the tool set as well then those files will also be included in the bundle.

### Windows

The `cmake --install ...` step will create a zip archive in `build/presets/<preset>/install/`.

If you built the tool set as well then those files will also be included in the archive. If using the `fs2` preset, the FRED2 mission editor will also be built as part of the tool set.
