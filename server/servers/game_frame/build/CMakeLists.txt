# cmake build list
# Copyright (C) 2013 toney
# @author: toney
# 2013-04-18
# gameServer cmake
cmake_minimum_required(VERSION 3.1)
project(gameServerFrame VERSION 1.0 LANGUAGES C CXX)

include(../../../cmake_config/config.cmake)
include(../../../cmake_config/macros.cmake)
include(../../../cmake_config/common_path.cmake)


clear_all_list()

add_include_path(../../../server_frame/include)
add_include_path(../../common/src)
add_include_path(../../pb/src)

add_include_path(${COMMON_INCLUDE})
add_include_path(${MYSQL_INCLUDE})
add_include_path(../../../server_frame/include/third/lua)
add_include_path(../../../server_frame/include/third)

add_include_path(../src)
add_source_files(../src/*.*)

add_link_path(../../../server_frame/lib)

add_link_path(${COMMON_LIBPATH})


build_static_lib(gameframe ../../../server_frame/lib)