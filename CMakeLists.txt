cmake_minimum_required(VERSION 3.16)

project(MyTest VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS QuickControls2)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMyTest
    main.cpp
)

qt_add_qml_module(appMyTest
    URI MyTest
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES folderwatcher.h folderwatcher.cpp
        QML_FILES InputText.qml
        QML_FILES MyButton.qml
        QML_FILES test.qml
    RESOURCES Icons/password.svg
              Icons/user.svg
              RESOURCES Icons/app.ico

)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appMyTest PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appMyTest
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMyTest
    PRIVATE Qt6::Quick
)
target_link_libraries(appMyTest PRIVATE Qt6::QuickControls2)

include(GNUInstallDirs)
install(TARGETS appMyTest
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
