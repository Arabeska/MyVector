cmake_minimum_required(VERSION 3.11)

project(Vector CXX)
set(CMAKE_CXX_STANDARD 17)

if(CMAKE_SYSTEM_NAME MATCHES "^MINGW")
    set(SYSTEM_LIBS -lstdc++)
else()
    set(SYSTEM_LIBS)
endif()

add_executable(my_vector main.cpp vector.h tests.h)

target_link_libraries(my_vector ${SYSTEM_LIBS})