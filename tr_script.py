import argparse
import subprocess


PROJECT = "headsetcontrol-qt"


def run_pylupdate():
    try:
        subprocess.run(
            [
                "pylupdate6.exe",
                f"./src/{PROJECT}.py",
                "./src/designer/ui_mainwindow.ui",
                "-ts",
                f"./src/tr/{PROJECT}_fr.ts",
                "-ts",
                f"./src/tr/{PROJECT}_de.ts",
                "-ts",
                f"./src/tr/{PROJECT}_es.ts",
                "-ts",
                f"./src/tr/{PROJECT}_en.ts",
            ],
            check=True,
        )
        print("pylupdate6 executed successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error running pylupdate6: {e}")


def run_lrelease():
    try:
        subprocess.run(
            [
                "lrelease.exe",
                f"./src/tr/{PROJECT}_de.ts",
                f"./src/tr/{PROJECT}_en.ts",
                f"./src/tr/{PROJECT}_es.ts",
                f"./src/tr/{PROJECT}_fr.ts",
            ],
            check=True,
        )
        print("lrelease executed successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error running lrelease: {e}")


def main():
    parser = argparse.ArgumentParser(description="Run pylupdate6 or lrelease.")
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--generate", "-g", action="store_true", help="Run pylupdate6")
    group.add_argument("--compile", "-c", action="store_true", help="Run lrelease")

    args = parser.parse_args()

    if args.generate:
        run_pylupdate()
    elif args.compile:
        run_lrelease()


if __name__ == "__main__":
    main()
