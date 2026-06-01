cmake_minimum_required(VERSION 3.19)
project(OneClone LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg Network)

qt_standard_project_setup()

set(CMAKE_AUTORCC ON)

qt_add_executable(OneClone
    WIN32 MACOSX_BUNDLE
    main.cpp
    model/Config.h
    model/SharedSettings.h      model/SharedSettings.cpp
    model/Job.h                 model/Job.cpp
    model/Manager.h             model/Manager.cpp
    view/MainWindow.h           view/MainWindow.cpp             view/MainWindow.ui
    view/JobWidget.h            view/JobWidget.cpp              view/JobWidget.ui
    resources.qrc
    main.rc
)

target_link_libraries(OneClone
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Network
        Qt::Svg
)

install(TARGETS OneClone
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET OneClone
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
