import sys
import os
from cx_Freeze import setup, Executable
import darkdetect

base = None

if sys.platform == "win32":
    base = "Win32GUI"
    include_files = ["icons/", "dependencies/", "design.ui"]
elif sys.platform == "linux":
    include_files = ["design.ui"]

build_exe_options = {
    "include_files": include_files,
    "excludes": [],
}

executables = [
    Executable(
        script="headsetcontrol-qt.py",
        base=base,
        icon=None,
        target_name="HeadsetControl-Qt",
    )
]

setup(
    name="HeadsetControl-Qt",
    version="1.0",
    options={"build_exe": build_exe_options},
    executables=executables
)
