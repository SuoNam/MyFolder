cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "")
endif()

project(MyFolder VERSION 1.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 Core Concurrent Network WebSockets Widgets)
find_package(LibDataChannel CONFIG REQUIRED)
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
        GroupFilesPage.qml
        CloudFilesPage.qml
        ServerFilesPage.qml
        SendToDialog.qml
        ServerUploadDialog.qml
        GroupTransferDialog.qml
        IncomingTransferToast.qml
        TransferCompletionToast.qml
        DeviceCard.qml
        DeviceDetailsDialog.qml
        TransferRow.qml
        RouteLadder.qml
        RailItem.qml
        UiButton.qml
        UiCheckBox.qml
        UiComboBox.qml
        UiPromptDialog.qml
        UiInput.qml
        UiChip.qml
        UiLed.qml
        UiProgress.qml
        UiNotice.qml
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
        lantransfermanager.h lantransfermanager.cpp
        p2ptransfermanager.h p2ptransfermanager.cpp
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
    PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Core Qt6::Concurrent Qt6::Network
    PRIVATE Qt6::WebSockets Qt6::Widgets
    PRIVATE LibDataChannel::LibDataChannel
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

include(CTest)
if(BUILD_TESTING)
    option(MYFOLDER_ENABLE_P2P_NETWORK_TESTS
        "Run local ICE/DataChannel tests (requires UDP loopback allowed by firewall)" OFF)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    qt_add_executable(lantransfermanager_test
        tests/lantransfermanager_test.cpp
        lantransfermanager.h lantransfermanager.cpp
        serverconfig.h
    )
    target_link_libraries(lantransfermanager_test
        PRIVATE Qt6::Core Qt6::Concurrent Qt6::Network Qt6::Test Qt6::Qml
    )
    add_test(NAME lantransfermanager_test COMMAND lantransfermanager_test)
    qt_add_executable(relaydownloadmanager_test
        tests/relaydownloadmanager_test.cpp
        relaydownloadmanager.h relaydownloadmanager.cpp
        serverconfig.h
    )
    target_link_libraries(relaydownloadmanager_test
        PRIVATE Qt6::Core Qt6::Network Qt6::Test Qt6::Qml
    )
    add_test(NAME relaydownloadmanager_test COMMAND relaydownloadmanager_test)
    qt_add_executable(forwardmanager_test
        tests/forwardmanager_test.cpp
        forwardmanager.h forwardmanager.cpp
        serverconfig.h
    )
    target_link_libraries(forwardmanager_test
        PRIVATE Qt6::Core Qt6::Network Qt6::Test Qt6::Qml
    )
    add_test(NAME forwardmanager_test COMMAND forwardmanager_test)
    qt_add_executable(httphandler_test
        tests/httphandler_test.cpp
        httphandler.h httphandler.cpp
        serverconfig.h
    )
    target_link_libraries(httphandler_test
        PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::Test
    )
    if(WIN32)
        target_link_libraries(httphandler_test PRIVATE Crypt32)
    endif()
    add_test(NAME httphandler_test COMMAND httphandler_test)
    qt_add_executable(p2ptransfermanager_test
        tests/p2ptransfermanager_test.cpp
        p2ptransfermanager.h p2ptransfermanager.cpp
        serverconfig.h
    )
    target_link_libraries(p2ptransfermanager_test
        PRIVATE Qt6::Core Qt6::Concurrent Qt6::Network Qt6::Test Qt6::Qml
        PRIVATE LibDataChannel::LibDataChannel
    )
    if(MYFOLDER_ENABLE_P2P_NETWORK_TESTS)
    add_test(NAME p2ptransfermanager_test COMMAND p2ptransfermanager_test)
    set_tests_properties(p2ptransfermanager_test PROPERTIES TIMEOUT 45 LABELS "network;p2p")
    endif()

    qt_add_executable(shellintegration_test
        tests/shellintegration_test.cpp
        shellintegration.h shellintegration.cpp
    )
    target_link_libraries(shellintegration_test PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
    add_test(NAME shellintegration_test COMMAND shellintegration_test)
    set_tests_properties(shellintegration_test PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

    qt_add_executable(transfermanager_test
        tests/transfermanager_test.cpp
        transfermanager.h transfermanager.cpp
        serverconfig.h
    )
    target_link_libraries(transfermanager_test
        PRIVATE Qt6::Core Qt6::Network Qt6::Test Qt6::Qml
    )
    add_test(NAME transfermanager_test COMMAND transfermanager_test)

    qt_add_executable(devicemanager_test
        tests/devicemanager_test.cpp
        devicemanager.h devicemanager.cpp
        deviceidentifier.h deviceidentifier.cpp
        serverconfig.h
    )
    target_link_libraries(devicemanager_test
        PRIVATE Qt6::Core Qt6::Network Qt6::Test Qt6::Qml
    )
    add_test(NAME devicemanager_test COMMAND devicemanager_test)
endif()
