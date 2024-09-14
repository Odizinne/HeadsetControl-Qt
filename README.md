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

### Windows

Precompiled binaries can be found in [release](https://github.com/odizinne/headsetControl-Qt/releases/latest) section.  
You need [Microsoft Visual C++ Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe) if it is not already installed.

### Linux

Linux binaries are built on ubuntu 24.04.  
You need Qt 6.4.2+ and latest [headsetcontrol](https://github.com/Sapd/HeadsetControl?tab=readme-ov-file#building) installed and available in path.  
Linux support is experimental and not well tested.

## To-do

- Add other headsetcontrol supported settings (My headset does not support them so i cannot test)
- Big code cleanup

## Credits

- [Sapd](https://github.com/Sapd/HeadsetControl) for headsetcontrol
- [bbacskay](https://github.com/bbacskay) Hungarian translation
