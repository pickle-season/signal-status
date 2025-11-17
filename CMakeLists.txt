cmake_minimum_required(VERSION 3.3)
project(signal-status)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

qt_standard_project_setup()
qt_add_executable(signal-status main.cpp
        Player.cpp
        Player.h
        DBusInterface.cpp
        DBusInterface.h)

target_link_libraries(signal-status PRIVATE Qt6::Core Qt::DBus)

