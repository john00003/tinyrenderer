cmake_minimum_required(VERSION 3.29)
project(mytinyrenderer)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

FetchContent_Declare(
        eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
        GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(eigen)

add_library(our_gl STATIC our_gl.cpp
        our_gl.h)

add_executable(mytinyrenderer main.cpp
        tgaimage.cpp
        tgaimage.h
        model.cpp
        model.h
        geometry.h)
        #main2.cpp)

target_link_libraries(our_gl PRIVATE Eigen3::Eigen)
target_link_libraries(mytinyrenderer PRIVATE Eigen3::Eigen)
target_link_libraries(mytinyrenderer PRIVATE our_gl)
