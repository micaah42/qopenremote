cmake_minimum_required(VERSION 3.22)

project(qopenremote LANGUAGES CXX VERSION 2.0)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAXE_CXX_FLAGS} -Wall -Werror")

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core WebSockets Gui)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core WebSockets Gui)

find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(${PROJECT_NAME} SHARED
    src/qopenremote_global.h

    src/qobjectregistry.cpp
    src/qobjectregistry.h
    src/websocketserver.cpp
    src/websocketserver.h
    src/jsonadapter.cpp
    src/jsonadapter.h
    src/listmodel.cpp
    src/listmodel.h
    src/json.cpp
    src/json.h

    src/setting.cpp
    src/setting.h
    src/sortfilterproxylistmodel.h src/sortfilterproxylistmodel.cpp
)

target_link_libraries(qopenremote PRIVATE Qt6::Core)


set_target_properties(${PROJECT_NAME} PROPERTIES SOVERSION ${PROJECT_VERSION})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/>
    $<INSTALL_INTERFACE:src/>
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::WebSockets
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QOPENREMOTE_LIBRARY
)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

include(CTest)

if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
