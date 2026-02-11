# Building the Project

Building requires [CMake](https://cmake.org) v3.25 or newer and [Ninja](https://ninja-build.org). All other dependencies will be satisfied as part of the general build process.

FreeSpace2 officially supports x64 and arm64 architectures for: Linux, macOS 11 or above, and Windows 10 or above. Other platforms may also work, but only those listed have been tested.

Using an IDE such as Xcode or Visual Studio is not covered in this document as they do not support CMake presets. Please reference the [CMakePresets.json](CMakePresets.json) file for the flags used for each preset if you'd like to configure with a generator other than Ninja.

## Presets
Presets are available to easily configure and build the project.

| Preset    | Description                 | Notes |
| --------- | --------------------------- | ----- |
| `fs2`     | **FreeSpace2**              |       |
| `fs2demo` | **FreeSpace2 Demo**         | Configure with `-DDEMO_GAME_DATA=<path_to_demo_files>` to allow creating a self-contained package |
| `fs1`     | **Descent: FreeSpace**      |       |
| `fs1demo` | **Descent: FreeSpace Demo** | Configure with `-DDEMO_GAME_DATA=<path_to_demo_files>` to allow creating a self-contained package |

## Build types

A build type will create release or debug binaries. If not specified, the build type is `RelWithDebInfo`.

| Build Type       | Description                                           |
| ---------------- | ----------------------------------------------------- |
| `RelWithDebInfo` | Release build with debug info. **Default**            |
| `Release`        | Release build stripped of debug info                  |
| `Debug`          | Debug build with logging and and extra error handling |

## Configure and Compile

### Linux

Using your package manager install the basic requirements, `cmake` and `ninja-build`. SDL has additional development packages which should be installed as well. Please see [README-linux](https://wiki.libsdl.org/SDL3/README-linux) from the SDL wiki for the needed dependencies.

To aid in building on Linux a Dockerfile is provided with the project (under `dist/docker/`) which includes all of the dependencies required. Please see the [README](dist/docker/README.md) for additional information.

Configure your desired preset with:
```bash
cmake --preset <preset>
```

And build it with:
```bash
cmake --build --preset <preset> --config <build-type>
```

If you would also like the command line toolset for working with game files then build them with:
```bash
cmake --build --preset <preset>  --target toolset --config <build-type>
```

An AppImage of the build can be generated with:
```bash
cmake --install build/presets/<preset> --config <build-type>
```

This will create an AppImage in `build/presets/<preset>/install/` that can be easily installed.

If you built the toolset as well then those files will also be included in the AppImage. Creating a symlink with the name of the tool pointing to the AppImage will allow easy use of each tool (e.g., `ln -s <game>.AppImage cfileutil`, then run `./cfileutil` to use the `.vp` file utility).

### macOS

Install the basic requirements using your preferred method (`homebrew`, manual install, etc.).

Configure your desired preset with:
```bash
cmake --preset <preset>
```

By default a build will be created for your current architecture (Intel or Apple Silicon). To use an alternate architecture add `-DCMAKE_OSX_ARCHITECTURES="x86_64"` (to build for Intel), or `-DCMAKE_OSX_ARCHITECTURES="arm64"` (to build for Apple Silicon) to the configure step.

> [!IMPORTANT]
> It is not currently possible to create a universal binary by specifying both architectures at the same time as dependencies must be configured for one architecture or the other. To create a universal binary you must compile and install the package for each architecture separately and then manually combine them into a univeral package using `lipo`.

Build the preset with:
```bash
cmake --build --preset <preset> --config <build-type>
```

If you would also like the command line toolset for working with game files then build them with:
```bash
cmake --build --preset <preset>  --target toolset --config <build-type>
```

An app bundle of the build can be generated with:
```bash
cmake --install build/presets/<preset> --config <build-type>
```

This will create an app bundle in `build/presets/<preset>/install/` that can be easily installed. If you built the toolset as well then those files will also be included in the bundle.

### Windows

Configure your desired preset with:
```bash
cmake --preset <preset>
```

Build the preset with:
```bash
cmake --build --preset <preset> --config <build-type>
```

If you would also like the command line toolset for working with game files then build them with:
```bash
cmake --build --preset <preset>  --target toolset --config <build-type>
```

A zip file of the build and required DLLs can be be generated with:
```bash
cmake --install build/presets/<preset> --config <build-type>
```

This will create a zip archive in `build/presets/<preset>/install/` that can be easily installed. If you built the toolset as well then those files will also be included in the archive.

On Windows, if using the `fs2` preset, the FRED2 mission editor will also be built as part of the toolset.
