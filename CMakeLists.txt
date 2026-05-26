cmake_minimum_required(VERSION 3.19)
project(SimpleRCloneMountGui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Svg)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/settingsmanager.cpp
    src/mountservice.cpp
    src/servicelistdelegate.cpp
    src/servicelistmodel.cpp
)

set(HEADERS
    src/mainwindow.h
    src/settingsmanager.h
    src/mountservice.h
    src/servicelistdelegate.h
    src/servicelistmodel.h
    src/appconfig.h
)

set(UI_FILES
    src/mainwindow.ui
)

set(RESOURCE_FILES
    resources/resources.qrc
)

add_executable(SimpleRCloneMountGui WIN32
    ${SOURCES}
    ${HEADERS}
    ${UI_FILES}
    ${RESOURCE_FILES}
)

target_link_libraries(SimpleRCloneMountGui
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)

target_include_directories(SimpleRCloneMountGui PRIVATE src)

