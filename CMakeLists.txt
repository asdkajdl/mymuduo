cmake_minimum_required(VERSION 3.22)
project(mymuduo)

set(CMAKE_CXX_STANDARD 17)

#add_executable(mymuduo main.cpp base/nocopyable.h base/CurrentThread.h base/CurrentThread.cpp base/Thread.h base/Thread.cpp base/ThreadPool.h base/ThreadPool.cpp logger/FixedBuffer.h)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/base BASE)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/http HTTP)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/logger LOGGER)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/net NET)
add_executable(mymuduo main.cpp ${BASE} ${HTTP} ${LOGGER} ${NET})