import sys
from cx_Freeze import setup, Executable
# PyQt6 import statement for proper freezing

base = None

if sys.platform == "win32":
    base = "Win32GUI"
    include_files = ["icons/", "dependencies/", "design.ui"]
    zip_include_packages = ["PyQt6"]
    if "winshell" in sys.modules:
        zip_include_packages.append("winshell")
elif sys.platform == "linux":
    include_files = ["design.ui"]
    zip_include_packages = ["PyQt6"]

build_exe_options = {
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