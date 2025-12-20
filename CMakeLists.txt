cmake_minimum_required(VERSION 3.15)
project(IJKMediaPlayer_Win VERSION 1.0.0 LANGUAGES C CXX)

# 设置C/C++标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

# 设置输出目录为output
set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/output)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# 对于Visual Studio生成器，需要设置每个配置的输出目录
if(CMAKE_GENERATOR MATCHES "Visual Studio")
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR})
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR})
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR})
    endforeach()
endif()

# 检测平台
if(WIN32)
    # 设置Windows特定选项
    add_definitions(-DWIN32 -D_MBCS -D_CRT_SECURE_NO_WARNINGS -DCOBJMACROS)
    
    # 检测架构
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(PLATFORM_ARCH "x64")
        set(PLATFORM_DIR "x64")
    else()
        set(PLATFORM_ARCH "Win32")
        set(PLATFORM_DIR "Win32")
    endif()
    
    # 设置VS2019工具集（如果可用）
    if(CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_GENERATOR_TOOLSET "v142" CACHE STRING "Visual Studio toolset")
    endif()
else()
    message(FATAL_ERROR "This CMakeLists.txt is for Windows platform only")
endif()

# 设置第三方库路径
set(THIRD_PARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set(FFMPEG_DIR ${THIRD_PARTY_DIR}/ffmpeg)
set(SDL2_DIR ${THIRD_PARTY_DIR}/sdl2)
set(PTHREAD_DIR ${THIRD_PARTY_DIR}/pthread-win32)
set(ILOG3_DIR ${THIRD_PARTY_DIR}/iLog3)

# 设置包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${FFMPEG_DIR}/include
    ${SDL2_DIR}/include
    ${PTHREAD_DIR}/include
    ${ILOG3_DIR}/include
)

# 设置库目录
if(PLATFORM_ARCH STREQUAL "x64")
    set(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib/x64)
    set(SDL2_LIB_DIR ${SDL2_DIR}/lib/x64)
    set(PTHREAD_LIB_DIR ${PTHREAD_DIR}/lib/x64)
    set(ILOG3_LIB_DIR ${ILOG3_DIR}/lib/x64)
else()
    set(FFMPEG_LIB_DIR ${FFMPEG_DIR}/lib/Win32)
    set(SDL2_LIB_DIR ${SDL2_DIR}/lib/Win32)
    set(PTHREAD_LIB_DIR ${PTHREAD_DIR}/lib/Win32)
    set(ILOG3_LIB_DIR ${ILOG3_DIR}/lib/Win32)
endif()

# 将库目录设置为全局变量，供子目录使用
set(FFMPEG_LIB_DIR ${FFMPEG_LIB_DIR} CACHE INTERNAL "FFmpeg library directory")
set(SDL2_LIB_DIR ${SDL2_LIB_DIR} CACHE INTERNAL "SDL2 library directory")
set(PTHREAD_LIB_DIR ${PTHREAD_LIB_DIR} CACHE INTERNAL "pthread library directory")
set(ILOG3_LIB_DIR ${ILOG3_LIB_DIR} CACHE INTERNAL "iLog3 library directory")

link_directories(
    ${FFMPEG_LIB_DIR}
    ${SDL2_LIB_DIR}
    ${PTHREAD_LIB_DIR}
    ${ILOG3_LIB_DIR}
)

# 添加子目录
add_subdirectory(src)
add_subdirectory(demo)

