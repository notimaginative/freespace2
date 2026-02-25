# FreeSpace 2

> [!NOTE]
> This is a continuation of the icculus.org [Freespace2](https://icculus.org/freespace2) project. This project is not supported by Volition or Interplay or icculus.org.

The goal of the ***FreeSpace 2*** project is to create a multi-platform port of the original FreeSpace 2 game from Volition while supporting newer operating systems and platforms where possible. The project maintains full compatibility with the original game and strives to preserve the original retail experience. Hardware requirements are kept fairly low and does not require a high-end machine to run.

This project also includes a backport of the code to allow running the original ***Descent: FreeSpace***.

This project is NOT concerned with adding new game features, supporting mods, or adding new graphics capabilities. If you are looking for those things please refer to the Source Code Project and FreeSpace Open in the [Links](#links) section below.

## Getting Started

### Requirements

**You must provide your own game files.**

Versions of ***FreeSpace 2*** and ***Descent: FreeSpace*** available from Steam or GOG are supported, as are the original CD-ROM and DVD releases. The game must already be installed on your system or be copied from a system which already has the game installed.

Download an [official release](https://github.com/notimaginative/freespace2/releases) or build one yourself (see [BUILDING](BUILDING.md)).

> [!TIP]
> Demo versions can be packaged to already contain the required game data. All official releases from this project will package demo versions ready to run and without needing extra files.

### Installation

There are three basic options for how to install game data. Please choose the one that works best for you.

1. Install the game binary wherever you like and run it, then go to Setup and click the Misc tab, and set the Extras Path to the location of your game data (*best for existing Steam/GOG install*)
2. Install the game binary wherever you like and run it once, then Quit. Copy your game data (`*.vp` and `data` folder) to your users data path (Linux: `~/.local/share/Volition/<game>`, macOS: `~/Library/Application Support/Volition/<game>`, Windows: `C:\Users\<you>\AppData\Roaming\Volition\<game>`)
3. Place the extracted binary package in the same location as your game data (***WARNING:** may overwrite retail files!!*)

## Toolset

A basic set of command line and GUI tools is provided to help create new content for FreeSpace 2. This includes tools such as *FRED*, the GUI mission editor, and `cfileutil`, a command line utility for creating `.vp` archives.

Please see [README-toolset](README-toolset.md) for information about the tools, what they do, and how to use them.

## Links

- Purchase **FreeSpace 2**: [Steam](https://store.steampowered.com/app/273620/Freespace_2/), [GOG](https://www.gog.com/game/freespace_2)
- Purchase **Descent: FreeSpace**: [Steam](https://store.steampowered.com/app/273600/Descent_FreeSpace__The_Great_War/), [GOG](https://www.gog.com/game/freespace_expansion)
- [PXO](https://pxo.nottheeye.com) (multiplayer matching service)
- icculus.org [Freespace2](https://icculus.org/freespace2) project
- [Hard Light Productions](https://www.hard-light.net) (for FreeSpace Open)
- [Source Code Project](https://scp.indiegames.us) (for FreeSpace Open)
- [FreeSpace Open](https://github.com/scp-fs2open/fs2open.github.com) on Github

## Project Status

What works (all of the basic game, including):
- [x] Full featured OpenGL ES 2 renderer
- [x] Cross-platform multiplayer over LAN and Internet
- [x] PXO multiplayer support
- [x] Original movies
- [x] Gamepad support
- [x] Haptic and Rumble support
- [x] Sound and music

What doesn't work:
- **Descent: FreeSpace** multiplayer is not compatible with retail version
- No IPv6 support (breaks compatibility with retail version)

In progress:
- [ ] FRED (for **Descent: FreeSpace**)
- [ ] FRED2 running under Wine as packaged app for Linux/macOS
- [ ] PofView imgui rewrite (model viewer, part of toolset)
- [ ] Emscripten port of demo versions
- [ ] New standalone server web UI
- [ ] Multicast for LAN games
