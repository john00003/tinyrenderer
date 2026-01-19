cmake_minimum_required(VERSION 3.29)
project(mytinyrenderer)

set(CMAKE_CXX_STANDARD 23)

add_executable(mytinyrenderer main.cpp
        tgaimage.cpp
        tgaimage.h
        model.cpp
        model.h
        geometry.h)
        #main2.cpp)
