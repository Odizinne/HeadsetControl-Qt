QT       += core gui qml quick quickcontrols2

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 silent lrelease embed_translations qtquickcompiler

QM_FILES_RESOURCE_PREFIX=/translations
DEPENDENCIES_DIR = $$PWD/dependencies
DEST_DIR = $$OUT_PWD/release/dependencies

INCLUDEPATH += \
    ShortcutManager \
    HeadsetControlQt \
    Utils \
    Worker \
    HIDEventMonitor

SOURCES += \
    ShortcutManager/ShortcutManager.cpp \
    Worker/Worker.cpp \
    HIDEventMonitor/HIDEventMonitor.cpp \
    main.cpp \
    HeadsetControlQt/HeadsetControlQt.cpp \
    Utils/Utils.cpp

HEADERS += \
    ShortcutManager/ShortcutManager.h \
    HeadsetControlQt/HeadsetControlQt.h \
    Utils/Utils.h \
    Worker/Worker.h \
    HIDEventMonitor/HIDEventMonitor.h

TRANSLATIONS += \
    Resources/tr/HeadsetControl-Qt_fr.ts \
    Resources/tr/HeadsetControl-Qt_en.ts \
    Resources/tr/HeadsetControl-Qt_hu.ts \
    Resources/tr/HeadsetControl-Qt_es.ts \
    Resources/tr/HeadsetControl-Qt_de.ts \
    Resources/tr/HeadsetControl-Qt_it.ts \
    Resources/tr/HeadsetControl-Qt_tr.ts

RESOURCES += \
    Resources/resources.qrc

RC_FILE = Resources/appicon.rc

win32 {
    LIBS += -luser32 -ladvapi32
}

unix {
    LIBS += -ludev
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32 {
    QMAKE_POST_LINK += powershell -Command "New-Item -ItemType Directory -Path '$$DEST_DIR' -Force; Copy-Item -Path '$$DEPENDENCIES_DIR\*' -Destination '$$DEST_DIR' -Recurse -Force"
}

