# HeadsetControl-Qt

[![Github All Releases](https://img.shields.io/github/downloads/odizinne/headsetcontrol-qt/total.svg)]()
[![license](https://img.shields.io/github/license/odizinne/headsetcontrol-qt)]()

Qt Gui for headsetcontrol; windows / linux compatible.  
Features a tray icon with battery status in tooltip.

## Overview

![image](assets/screenshot.png)

## Supported devices

Supported devices list can be found [here](https://github.com/Sapd/HeadsetControl?tab=readme-ov-file#supported-headsets).  
If a particular setting is greyed out on the settings page, it indicates that your device does not support it.

## Download

Precompiled binaries can be found in [release](https://github.com/odizinne/headsetControl-Qt/releases/latest) section.  

### Windows

You need [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) if it is not already installed.

### Linux

Linux binaries are built on ubuntu 24.04 with Qt 6.8.2.
Qt 6.8 required
You need latest [headsetcontrol](https://github.com/Sapd/HeadsetControl?tab=readme-ov-file#building) available in path. 

### Installation on Arch Linux

Arch Linux users can install the application using `makepkg`.

First install `headsetcontrol` as a dependency. 
```bash
yay -S --asdeps headsetcontrol-git
```
Then, build and install the application:

```bash
makepkg -si
```
The application is now ready to use.
```bash
 headsetcontrol-qt
```

## To-do

- Add other headsetcontrol supported settings (My headset does not support them so i cannot test)
- Big code cleanup

## Translation

Currently done:

| Language     | Short code   | Made by                                    |
|--------------|--------------|--------------------------------------------|
| English      | EN           | Odizinne                                   |
| French       | FR           | Odizinne                                   |
| Hungarian    | HU           | [bbacskay](https://github.com/bbacskay)    |
| Turkish      | TR           | [DogancanYr](https://github.com/DogancanYr)|
| Spanish      | ES           | AI                                         |
| German       | DE           | AI                                         |
| Italian      | IT           | AI                                         |


## Want to help?

HeadsetControl-Qt is not available for your language?  
AI made a bad translation?  
Battery icons default to dark theme on a dark panel?

Feel free to sumbit a pull request or open an issue so we can work arround this!

## Credits

- [Sapd](https://github.com/Sapd/HeadsetControl) for headsetcontrol
- [jakears93](https://github.com/jakears93/Nova7ChatmixLinux) for Nova7ChatmixLinux scripts
