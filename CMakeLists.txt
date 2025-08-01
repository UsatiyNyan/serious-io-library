cmake_minimum_required(VERSION 3.21)

include(FetchContent)
FetchContent_Declare(
        serious-cmake-library
        GIT_REPOSITORY git@github.com:UsatiyNyan/serious-cmake-library.git
        GIT_TAG main
)
FetchContent_MakeAvailable(serious-cmake-library)

sl_project_prologue(
    C_STANDARD 17
    CXX_STANDARD 23)

project(
        serious-io-library
        VERSION 0.0.1
        DESCRIPTION "async sockets, epoll exclusively for linux for now"
        HOMEPAGE_URL "TODO"
        LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC
    src/sys/file.cpp
    src/sys/epoll.cpp
    src/sys/socket.cpp
    src/state/socket.cpp
    src/state/server.cpp
    src/async/socket.cpp
    src/async/server.cpp
)

add_library(sl::io ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC include)

add_subdirectory(dependencies)

if (NOT PROJECT_IS_TOP_LEVEL)
    return()
endif ()

sl_compiler_warnings(${PROJECT_NAME} WARNINGS_AS_ERRORS)

add_subdirectory(examples)

include(CTest)
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()
