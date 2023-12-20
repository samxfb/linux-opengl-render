cmake_minimum_required(VERSION 3.5)

message("############# build demo  ############")

project(demo)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_VERBOSE_MAKEFILE ON)

include_directories("${PROJECT_SOURCE_DIR}")

file(GLOB SRC ${PROJECT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} -Wl,--start-group)
target_link_libraries(${PROJECT_NAME} -Wl,--exclude-libs,ALL)
target_link_libraries(${PROJECT_NAME} -Wl,-rpath,./)
target_link_libraries(${PROJECT_NAME} GL)
target_link_libraries(${PROJECT_NAME} X11)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} -Wl,--end-group)


file(COPY ${PROJECT_SOURCE_DIR}/1280x720_20fps_I420.yuv DESTINATION ${PROJECT_BINARY_DIR})