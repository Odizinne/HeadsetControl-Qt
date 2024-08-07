import os
from cx_Freeze import setup, Executable

src_dir = os.path.dirname(os.path.abspath(__file__))
build_dir = "build/HeadsetControl-Qt"
install_dir = os.path.join(os.getenv("LOCALAPPDATA"), "programs", "HeadsetControl-Qt")

include_files = [os.path.join(src_dir, "dependencies/"), os.path.join(src_dir, "icons"), os.path.join(src_dir, "tr/")]

zip_include_packages = ["PyQt6"]

build_exe_options = {
    "build_exe": build_dir,
    "include_files": include_files,
    "zip_include_packages": zip_include_packages,
    "excludes": ["tkinter"],
    "silent": True,
}

executables = [
    Executable(
        script=os.path.join(src_dir, "headsetcontrol-qt.py"),
        base="Win32GUI",
        icon=os.path.join(src_dir, "icons/icon.ico"),
        target_name="HeadsetControl-Qt",
    )
]

setup(
    name="HeadsetControl-Qt",
    version="1.0",
    options={"build_exe": build_exe_options},
    executables=executables,
)
