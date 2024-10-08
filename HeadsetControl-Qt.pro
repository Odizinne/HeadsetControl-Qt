QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    src/ShortcutManager \
    src/HeadsetControlQt \
    src/Utils \
    src/Worker

SOURCES += \
    src/ShortcutManager/shortcutmanager.cpp \
    src/Worker/worker.cpp \
    src/main.cpp \
    src/HeadsetControlQt/headsetcontrolqt.cpp \
    src/Utils/utils.cpp

HEADERS += \
    src/ShortcutManager/shortcutmanager.h \
    src/HeadsetControlQt/headsetcontrolqt.h \
    src/Utils/utils.h \
    src/Worker/worker.h

FORMS += \
    src/HeadsetControlQt/headsetcontrolqt.ui

TRANSLATIONS += \
    src/Resources/tr/HeadsetControl-Qt_fr.ts \
    src/Resources/tr/HeadsetControl-Qt_en.ts \
    src/Resources/tr/HeadsetControl-Qt_hu.ts \
    src/Resources/tr/HeadsetControl-Qt_es.ts \
    src/Resources/tr/HeadsetControl-Qt_de.ts \
    src/Resources/tr/HeadsetControl-Qt_it.ts

RESOURCES += \
    src/Resources/resources.qrc

RC_FILE = src/Resources/appicon.rc

# Build/embed translations automatically
CONFIG += lrelease
QM_FILES_RESOURCE_PREFIX=/translations/tr
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Define the source directory and the target directory in the build folder
DEPENDENCIES_DIR = $$PWD/dependencies
DEST_DIR = $$OUT_PWD/release/dependencies

win32 {
    QMAKE_POST_LINK += powershell -Command "New-Item -ItemType Directory -Path '$$DEST_DIR' -Force; Copy-Item -Path '$$DEPENDENCIES_DIR\*' -Destination '$$DEST_DIR' -Recurse -Force"
}

