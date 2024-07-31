import os
from cx_Freeze import setup, Executable
from setuptools.command.install import install as _install
import winshell

src_dir = os.path.dirname(os.path.abspath(__file__))
build_dir = "build/HeadsetControl-Qt"
install_dir = os.path.join(os.getenv("LOCALAPPDATA"), "programs", "HeadsetControl-Qt")

include_files = [os.path.join(src_dir, "dependencies/"), os.path.join(src_dir, "icons")]

zip_include_packages = ["PyQt6"]

build_exe_options = {
    "build_exe": build_dir,
    "include_files": include_files,
    "zip_include_packages": zip_include_packages,
    "excludes": ["tkinter"],
}

executables = [
    Executable(
        script=os.path.join(src_dir, "headsetcontrol-qt.py"),
        base="Win32GUI",
        icon=os.path.join(src_dir, "icons/icon.ico"),
        target_name="HeadsetControl-Qt",
    )
]


class InstallCommand(_install):
    def run(self):
        if not os.path.exists(install_dir):
            os.makedirs(install_dir)
        if not os.path.exists(build_dir):
            print("##################################################")
            print("# Nothing to install.                            #")
            print("# Please build the project first.                #")
            print("##################################################")
            return
        self.copy_tree(build_dir, install_dir)
        print("")
        print(f"Executable installed to {install_dir}")

        shortcut_path = os.path.join(winshell.programs(), "HeadsetControl-Qt.lnk")
        target = os.path.join(install_dir, "HeadsetControl-Qt.exe")
        icon = os.path.join(install_dir, "icons/icon.ico")

        winshell.CreateShortcut(
            Path=shortcut_path, Target=target, Icon=(icon, 0), Description="HeadsetControl-Qt", StartIn=install_dir
        )

        print("Created shortcut in start menu")


setup(
    name="HeadsetControl-Qt",
    version="1.0",
    options={"build_exe": build_exe_options},
    executables=executables,
    cmdclass={"install": InstallCommand},
)
