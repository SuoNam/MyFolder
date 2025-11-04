cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE "G:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
#set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic")
set(VCPKG_TARGET_TRIPLET "x64-windows")
project(MyTest VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")

if (MSVC)
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>"
        "$<$<COMPILE_LANGUAGE:CXX>:/permissive->"
        "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>"
    )
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(Tesseract_DIR "G:/vcpkg/installed/x64-windows-static/share/tesseract")
# set(OpenCV_DIR "G:/vcpkg/installed/x64-windows/share/opencv4")
find_package(Qt6 REQUIRED COMPONENTS Quick)
find_package(Qt6 REQUIRED COMPONENTS QuickControls2)
find_package(Qt6 REQUIRED COMPONENTS Core)
find_package(OpenCV CONFIG REQUIRED)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appMyTest
    main.cpp
    ocr.cpp
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
              SOURCES ocr.h ocr.cpp
              QML_FILES mainWindow.qml

)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appMyTest PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appMyTest
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appMyTest
    PRIVATE Qt6::Quick
    PRIVATE ${OpenCV_LIBS}
)
target_link_libraries(appMyTest PRIVATE Qt6::QuickControls2)
target_link_libraries(appMyTest PRIVATE Qt6::Core)

include(GNUInstallDirs)
install(TARGETS appMyTest
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
