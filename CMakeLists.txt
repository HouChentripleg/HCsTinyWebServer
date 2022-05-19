cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(HCSTINYWEBSERVER LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS -pthread)

set(SRC ./src/buffer.cpp 
        ./src/epoll.cpp 
        ./src/timer.cpp 
        ./src/httprequest.cpp 
        ./src/httpresponse.cpp 
        ./src/httpconnect.cpp 
        ./src/webserver.cpp)
set(INCLUDE ./include/buffer.hpp 
            ./include/epoll.hpp 
            ./include/timer.hpp 
            ./include/threadpool.hpp 
            ./include/httprequest.hpp 
            ./include/httpresponse.hpp 
            ./include/httpconnect.hpp 
            ./include/webserver.hpp)

add_library(TinyWebServer SHARED ${SRC} ${INCLUDE})
add_executable(HCsTinyWebServer main.cpp)
target_link_libraries(HCsTinyWebServer TinyWebServer)