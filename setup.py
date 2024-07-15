import sys
from cx_Freeze import setup, Executable

build_dir = "build/HeadsetControl-Qt"

base = "Win32GUI"
include_files = ["dependencies/", "icons"]
zip_include_packages = ["PyQt6", "winshell", "pywin32", "darkdetect"]
icon = "icons/icon.ico"

build_exe_options = {
    "build_exe": build_dir,
    "include_files": include_files,
    "zip_include_packages": zip_include_packages,
    "excludes": [],
}

executables = [
    Executable(
        script="headsetcontrol-qt.py",
        base=base,
        icon=icon,
        target_name="HeadsetControl-Qt",
    )
]

setup(
    name="HeadsetControl-Qt",
    version="1.0",
    options={"build_exe": build_exe_options},
    executables=executables
)