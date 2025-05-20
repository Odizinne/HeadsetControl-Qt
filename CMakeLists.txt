cmake_minimum_required(VERSION 3.16)
project(HeadsetControl-Qt VERSION 1.0 LANGUAGES C CXX)
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory" FORCE)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(QT_QML_GENERATE_QMLLS_INI ON)
set(CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders TRUE)

# Add this line to include GNUInstallDirs
include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
    QuickControls2
    Widgets
    LinguistTools
)
qt_standard_project_setup(REQUIRES 6.9)
set(SOURCES
    src/shortcutmanager.cpp
    src/worker.cpp
    src/headsetcontrolqt.cpp
    src/main.cpp
    src/utils.cpp
    src/hideventmonitor.cpp
    src/headsetcontrolchecker.cpp
)
set(HEADERS
    include/shortcutmanager.h
    include/headsetcontrolqt.h
    include/utils.h
    include/worker.h
    include/hideventmonitor.h
    include/headsetcontrolchecker.h
)
qt_add_executable(HeadsetControl-Qt WIN32 MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    Resources/resources.qrc
    Resources/appicon.rc
)
qt_add_qml_module(HeadsetControl-Qt
    URI Odizinne.HeadsetControlQt
    VERSION ${PROJECT_VERSION}
    QML_FILES
    qml/HeadsetcontrolPrompt.qml
    qml/Main.qml
)
target_include_directories(HeadsetControl-Qt PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
if(WIN32)
    target_link_libraries(HeadsetControl-Qt PRIVATE
        user32
        advapi32
    )
elseif(UNIX AND NOT APPLE)
    target_link_libraries(HeadsetControl-Qt PRIVATE
        udev
    )
endif()
target_link_libraries(HeadsetControl-Qt PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
    Qt::QuickControls2
    Qt::Widgets
)
qt_add_translations(HeadsetControl-Qt
    TS_FILES
        i18n/HeadsetControl-Qt_fr.ts
        i18n/HeadsetControl-Qt_en.ts
        i18n/HeadsetControl-Qt_hu.ts
        i18n/HeadsetControl-Qt_es.ts
        i18n/HeadsetControl-Qt_de.ts
        i18n/HeadsetControl-Qt_it.ts
        i18n/HeadsetControl-Qt_tr.ts
    RESOURCE_PREFIX "/i18n"
    LUPDATE_OPTIONS "-no-obsolete"
)
add_dependencies(${CMAKE_PROJECT_NAME} update_translations)

# Change this line to match QuickSoundSwitcher
install(TARGETS HeadsetControl-Qt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET HeadsetControl-Qt
    OUTPUT_SCRIPT deploy_script
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --no-compiler-runtime --no-opengl-sw --no-system-dxc-compiler --no-system-d3d-compiler --skip-plugin-types imageformats,designer,iconengines,qmllint,generic,networkinformation,help,qmltooling,sqldrivers,qmlls
)
install(SCRIPT ${deploy_script})
