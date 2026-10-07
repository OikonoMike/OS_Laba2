cmake_minimum_required(VERSION 3.10)

project(OS_Lab2 VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)

add_executable(parent parent.c)

add_executable(child_1 child_1.c)

add_executable(child_2 child_2.c)

if(WIN32)
    target_link_libraries(parent advapi32)
    target_link_libraries(child_1 advapi32)
    target_link_libraries(child_2 advapi32)
endif()