cmake_minimum_required(VERSION 3.14)
project(vld-0.16.0)
message("Begin cmaking of PHP extension ...")

include_directories(
        "$ENV{PHP_DEVEL}/include"
        "$ENV{PHP_DEVEL}/include/ext"
        "$ENV{PHP_DEVEL}/include/main"
        "$ENV{PHP_DEVEL}/include/sapi"
        "$ENV{PHP_DEVEL}/include/TSRM"
        "$ENV{PHP_DEVEL}/include/win32"
        "$ENV{PHP_DEVEL}/include/Zend"
)
link_directories("$ENV{PHP_PATH}/dev")

add_compile_definitions(
        _WINDOWS
        WINDOWS=1
        ZEND_WIN32=1
        PHP_WIN32=1
        WIN32
        _MBCS
        _USE_MATH_DEFINES
        # ZEND_WIN32_FORCE_INLINE
        ZTS=1
        ZEND_DEBUG=0
        COMPILE_DL_VLD_EXT
        ZEND_ENABLE_STATIC_TSRMLS_CACHE=0
)

file(GLOB SRC_FOLDER "./*.c" "./*.h")

#set(SOURCE_FILES
#        kernel/main.cpp
#)

add_library(vld_lib SHARED ${SRC_FOLDER})

message("End cmaking of PHP extension ...")