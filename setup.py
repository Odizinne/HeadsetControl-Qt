import sys
from cx_Freeze import setup, Executable

base = None
build_dir = "build/HeadsetControl-Qt"

if sys.platform == "win32":
    base = "Win32GUI"
    include_files = ["dependencies/", "battery_icons"]
    zip_include_packages = ["PyQt6", "winshell", "pywin32", "darkdetect"]
elif sys.platform == "linux":
    include_files = ["battery_icons"]
    zip_include_packages = ["PyQt6"]

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