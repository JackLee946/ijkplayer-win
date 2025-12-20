# ijkDemo - 简单示例程序
add_executable(ijkDemo
    ijkDemo/main.cpp
)

# 链接ijkwin库
target_link_libraries(ijkDemo
    ijkwin
)

# 设置包含目录
target_include_directories(ijkDemo PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# 设置编译选项
target_compile_options(ijkDemo PRIVATE
    /W3
)

# 禁用SAFESEH检查（FFmpeg库不支持）
target_link_options(ijkDemo PRIVATE
    /SAFESEH:NO
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ijkDemo PRIVATE DEBUG)
else()
    target_compile_definitions(ijkDemo PRIVATE NDEBUG)
endif()

# 复制DLL文件到输出目录
if(PLATFORM_ARCH STREQUAL "x64")
    set(DLL_PLATFORM_DIR "x64")
else()
    set(DLL_PLATFORM_DIR "Win32")
endif()

add_custom_command(TARGET ijkDemo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/avcodec-57.dll"
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/avformat-57.dll"
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/avutil-55.dll"
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/avfilter-6.dll"
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/swresample-2.dll"
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/swscale-4.dll"
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/postproc-54.dll"
        "${CMAKE_SOURCE_DIR}/third_party/ffmpeg/lib/${DLL_PLATFORM_DIR}/avdevice-57.dll"
        "${CMAKE_SOURCE_DIR}/third_party/sdl2/lib/${DLL_PLATFORM_DIR}/SDL2.dll"
        "${CMAKE_SOURCE_DIR}/third_party/pthread-win32/lib/${DLL_PLATFORM_DIR}/pthreadVC2.dll"
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Copying DLL files to output directory"
)

