import platform


def is_windows_10():
    os_version = platform.version()
    os_name = platform.system()

    # Windows 10 versions are generally greater than 10.0.10240
    # This can be fine-tuned as needed
    print(f"{os_name}, {os_version}")
    return os_name == "Windows" and os_version.startswith("10")
