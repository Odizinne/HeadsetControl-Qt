import platform


def is_windows_10():
    os_release = platform.release()
    os_name = platform.system()
    return os_name == "Windows" and os_release == "10"
