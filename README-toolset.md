# FreeSpace 2 Toolset

A set of tools is included with the FreeSpace 2 source code that can be used to work with game files or create new content. Some of these tools are of very limited use, have been replaced by more featured versions or rewritten for cross-platform support, and some tools have far superior replacements created by the modding community.

## Tools Available

- [ac](#ac) - AVI to ANI converter [^engine]
- [cfileutil](#cfileutil) - VP archive creator
- [cryptstring](#cryptstring) - Encrypt strings to be used as cheat codes
- [fonttool](#fonttool) - For creating and kerning VF font files [^engine]
- [FRED](#fred) - GUI mission and campaign editor/creator [^engine]
- [nebedit](#nebedit) - Create or edit ***Descent: FreeSpace** style nebula backgrounds [^engine]
- [scramble](#scramble) - Encrypt/Decrypt mission and table files

### `ac`

This command line utility can be used to convert AVI files to ANI for use in the game. `ac` is largely useless as it only supports a specific subset of AVI files which exceptionally few (if any) editors support any longer. It's provided purely as a cross-platform port of the retail utility used by Volition.

> [!TIP]
> A Windows GUI app called AniBuild32, which is part of [Descent Manager Tools](https://wiki.hard-light.net/index.php/Descent_Manager_Tools), is a far better tool for making ANI files. It also runs very well under Wine for Linux and Mac users.

### `cfileutil`

CFileUtil is a command line utility that can be used to create, extract, or list the contents of VP archives (`.vp`). This is a new utility which replaces the original `cfilearchiver` which was included with the original source code release and only supported creating VP archives.

> [!TIP]
> Several GUI based tools created by the modding community for working with VP archives are also available.

### `cryptstring`

`cryptstring` is a command line utility that takes one or more strings as arguments and outputs a line of encrypted text for each string that can be used in the source code to obfiscate cheat codes. The strings should not contain spaces or any character which requires a key combination to enter. Which means that only a-z, 0-9, and limited punctuation are allowed.

### `fonttool`

The FontTool command line utility can create a VF font file from a correctly formatted PCX file. It also has a simple GUI mode for font kerning of VF font files.

### FRED

FRED is primary method to create new content for the game. It allows you to create new campaigns and missions for single or multiplayer. FRED is a Windows GUI application, however it runs quite well under Wine for Linux and Mac users.

The terms *FRED* and *FRED2* are often used interchangeably, however *FRED* is the name of the editor for ***Descent: FreeSpace*** and *FRED2* is the name of the editor for ***FreeSapce 2***. Missions created with *FRED2* are not compatible with ***Descent: FreeSpace*** and vise versa.

And online copy of the documention for using FRED2 is available [here](https://fredzone.hard-light.net/freddocs/editors.html).

### `nebedit`

NebEdit is a simple GUI application which creates or edits ***Descent: FreeSpace*** style nebula backgrounds (`.neb` files). Proper use of this app is not documented.

***FreeSpace 2*** switched to bitmap rendering for backgrounds, which can be set inside *FRED2*, so this utility isn't used there.

### `scramble`

`scramble` is an encryption/decryption utility for the text-based mission and table files used by the game. Newer versions of the games available on Steam and GOG.com often do not include encrypted files, but older CD-ROM versions do (***Descent: FreeSpace*** in particular).

> [!IMPORTANT]
> Please note that the encryption method used by ***Descent: FreeSpace*** differs from the one used by ***FreeSpace 2***. So a version of `scramble` built with `FS1=ON` will not be able to encrypt or decrypt files for ***FreeSpace 2***, and vise versa.

[^engine]: This tool is compiled to include the game engine for basic functionality. As such it has the same library requirements in order to run and cannot be used as a standalone binary.
