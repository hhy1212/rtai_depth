cmake_minimum_required(VERSION 2.8)
project(rtdepth_shm)
add_executable(rtdepth rt_process.cpp)
add_executable(in in.cpp)
add_executable(out out.cpp)
include_directories(/usr/realtime/include)
find_package(OpenCV REQUIRED)
target_link_libraries(rtdepth ${OpenCV_LIBS} /usr/realtime/lib/liblxrt.a pthread)
target_link_libraries(in ${OpenCV_LIBS})
target_link_libraries(out ${OpenCV_LIBS})
