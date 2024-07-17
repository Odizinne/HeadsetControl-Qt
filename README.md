# HeadsetControl-Qt
PyQt6 Gui for headsetcontrol.<br/>
Windows / linux compatible.

![image](assets/screenshot.png)

Features a tray icon with battery status in tooltip.<br/>
Lights state will be restored on next start.

## Download
Precompiled windows binaries can be found in [release](https://odizinne.net/Projects/HeadsetControl-Qt/releases) section

## Build and run

Clone this repository: `git clone https://github.com/Odizinne/HeadsetControl-Qt.git`  
CD to the cloned repo: `cd HeadsetControl-Qt`

**Windows**  

Install dependencies:  
`pip install -r requirements.txt`

Build exe:  
`python .\src\setup.py build`

Install directory and create startup shortcut:  
`python .\src\setup.py install`

**Linux**  
Dependencies:
- PyQt6 (from package manager is recommended, else use pip)
- [HeadsetControl](https://github.com/Sapd/HeadsetControl)

There is nothing to build.

Make sure `headsetcontrol-qt.py` is executable: `chmod +x ./headsetcontrol-qt.py`  
Then run it: `headsetcontrol-qt.py`

## To-do
- Add other headsetcontrol supported settings (My headset does not support them so i cannot test)
