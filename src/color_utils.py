from PyQt6.QtGui import QColor, QPalette, QBrush


def set_frame_color_based_on_window(window, frame):
    def adjust_color(color, factor):
        r, g, b, a = color.red(), color.green(), color.blue(), color.alpha()
        r = min(max(int(r * factor), 0), 255)
        g = min(max(int(g * factor), 0), 255)
        b = min(max(int(b * factor), 0), 255)
        return QColor(r, g, b, a)

    def is_dark_mode(color):
        r, g, b = color.red(), color.green(), color.blue()
        brightness = (r + g + b) / 3
        return brightness < 127

    main_bg_color = window.palette().color(QPalette.ColorRole.Window)

    if is_dark_mode(main_bg_color):
        frame_bg_color = adjust_color(main_bg_color, 1.5)
    else:
        frame_bg_color = adjust_color(main_bg_color, 0.95)

    palette = frame.palette()
    palette.setBrush(QPalette.ColorRole.Window, QBrush(frame_bg_color))
    frame.setAutoFillBackground(True)
    frame.setPalette(palette)
