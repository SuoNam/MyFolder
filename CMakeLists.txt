cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()

project(MyFolder VERSION 1.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Core Network WebSockets Widgets)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMyFolder main.cpp)
target_compile_definitions(appMyFolder PRIVATE MYFOLDER_VERSION="${PROJECT_VERSION}")

if(WIN32)
    target_sources(appMyFolder PRIVATE app.rc)
endif()

# Singletons must be declared before the module.
set_source_files_properties(GlobalStatus.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(Config.qml       PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(Theme.qml        PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(appMyFolder
    URI MyTest
    VERSION 1.0
    QML_FILES
        Main.qml
        MainWindow.qml
        DevicesPage.qml
        TransfersPage.qml
        HistoryPage.qml
        SettingsPage.qml
        GroupsPage.qml
        SendToDialog.qml
        ServerUploadDialog.qml
        IncomingTransferToast.qml
        DeviceCard.qml
        DeviceDetailsDialog.qml
        TransferRow.qml
        RouteLadder.qml
        RailItem.qml
        UiButton.qml
        UiInput.qml
        UiChip.qml
        UiLed.qml
        UiProgress.qml
        QML_FILES GlobalStatus.qml
        QML_FILES Config.qml
        QML_FILES Theme.qml
    SOURCES
        folderwatcher.h folderwatcher.cpp
        httphandler.h httphandler.cpp
        websocketclient.h websocketclient.cpp
        transfermanager.h transfermanager.cpp
        devicemanager.h devicemanager.cpp
        forwardmanager.h forwardmanager.cpp
        relaydownloadmanager.h relaydownloadmanager.cpp
        deviceidentifier.h deviceidentifier.cpp
        shellintegration.h shellintegration.cpp
        serverconfig.h
    RESOURCES
        Icons/app.ico
        Icons/app.png
)

set_target_properties(appMyFolder PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMyFolder
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Core Qt6::Network
    PRIVATE Qt6::WebSockets Qt6::Widgets
)

if(WIN32)
    target_link_libraries(appMyFolder PRIVATE Crypt32)

    add_library(MyFolderShellExtension SHARED
        shell_extension.cpp
        shell_extension.def
    )
    target_link_libraries(MyFolderShellExtension PRIVATE Ole32 Shell32 Shlwapi Advapi32)
    if(MINGW)
        target_link_options(MyFolderShellExtension PRIVATE -static-libgcc -static-libstdc++)
    endif()
    set_target_properties(MyFolderShellExtension PROPERTIES
        PREFIX ""
        OUTPUT_NAME "MyFolderShellExtension"
    )
endif()

include(GNUInstallDirs)
install(TARGETS appMyFolder
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
