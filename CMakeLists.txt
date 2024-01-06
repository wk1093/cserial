cmake_minimum_required(VERSION 3.26)
project(cdata)

set(CMAKE_CXX_STANDARD 17)

add_executable(cdata main.cpp
        dtc/Type.h
        dtc/Variable.h
        dtc/Type.cpp
        dtc/Variable.cpp
        dtc/VarMath.cpp
        dtc/VarMath.h
        dtc/DynTypC.h
        dtc/Serial.h
        dtc/Serial.cpp
        dtc/DynTypC.cpp)
