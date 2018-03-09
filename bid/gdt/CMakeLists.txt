PROJECT (gdt)
cmake_minimum_required(VERSION 2.6)
SET(TARGET_EXE odin_gdt )
SET(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -DDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")

# Find required protobuf package
find_package(Protobuf REQUIRED)
if(PROTOBUF_FOUND)
    message(STATUS "protobuf library found")
else()
    message(FATAL_ERROR "protobuf library is needed but cant be found")
endif()
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS gdt_rtb.proto)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLIB_PKG glib-2.0)

IF (GLIB_PKG_FOUND)
    message("Found " glib-2.0)
ELSE (GLIB_PKG_FOUND)
    message("Not found glib-2.0")
ENDIF()
include_directories(${GLIB_PKG_INCLUDE_DIRS} "/usr/local/include/librdkafka/" ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
message("Found include " ${GLIB_PKG_INCLUDE_DIRS})
message("Found libraries " ${GLIB_PKG_LIBRARIES})

AUX_SOURCE_DIRECTORY(${PROTO_SRCS} SRC_LIST)
AUX_SOURCE_DIRECTORY(. SRC_LIST)
AUX_SOURCE_DIRECTORY(../../common SRC_LIST)
AUX_SOURCE_DIRECTORY(../../common/serverLog SRC_LIST)
AUX_SOURCE_DIRECTORY(../../common/baselib SRC_LIST)
AUX_SOURCE_DIRECTORY(../../common/xRedis SRC_LIST)

ADD_EXECUTABLE(${TARGET_EXE} ${SRC_LIST} ${PROTO_SRCS} ${PROTO_HDRS})
TARGET_LINK_LIBRARIES(${TARGET_EXE} pthread hiredis fcgi crypto rt ssl z protobuf rdkafka rdkafka++ ${GLIB_PKG_LIBRARIES})

install(TARGETS ${TARGET_EXE} DESTINATION "/usr/bin/")
