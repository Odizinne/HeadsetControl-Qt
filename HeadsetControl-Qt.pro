QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
    ShortcutManager \
    HeadsetControlQt \
    Utils \
    Worker

SOURCES += \
    ShortcutManager/ShortcutManager.cpp \
    Worker/Worker.cpp \
    main.cpp \
    HeadsetControlQt/HeadsetControlQt.cpp \
    Utils/Utils.cpp

HEADERS += \
    ShortcutManager/ShortcutManager.h \
    HeadsetControlQt/HeadsetControlQt.h \
    Utils/Utils.h \
    Worker/Worker.h

FORMS += \
    HeadsetControlQt/HeadsetControlQt.ui

TRANSLATIONS += \
    Resources/tr/HeadsetControl-Qt_fr.ts \
    Resources/tr/HeadsetControl-Qt_en.ts \
    Resources/tr/HeadsetControl-Qt_hu.ts \
    Resources/tr/HeadsetControl-Qt_es.ts \
    Resources/tr/HeadsetControl-Qt_de.ts \
    Resources/tr/HeadsetControl-Qt_it.ts

RESOURCES += \
    Resources/resources.qrc

RC_FILE = Resources/appicon.rc

# Build/embed translations automatically
CONFIG += lrelease
QM_FILES_RESOURCE_PREFIX=/translations/tr
CONFIG += embed_translations

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Define the source directory and the target directory in the build folder
DEPENDENCIES_DIR = $$PWD/dependencies
DEST_DIR = $$OUT_PWD/release/dependencies

win32 {
    QMAKE_POST_LINK += powershell -Command "New-Item -ItemType Directory -Path '$$DEST_DIR' -Force; Copy-Item -Path '$$DEPENDENCIES_DIR\*' -Destination '$$DEST_DIR' -Recurse -Force"
}

