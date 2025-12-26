# IJKPlayer - 专业音视频播放器
add_executable(IJKPlayer WIN32
    main.cpp
    IJKPlayerWindow.cpp
    PlayerController.cpp
    VideoRenderer.cpp
    PlaylistManager.cpp
    logging.cpp
    MenuWnd.cpp
    NetworkStreamDialog.cpp
)

# 链接库
target_link_libraries(IJKPlayer
    ijkwin
    duilib
    SDL2
    avcodec
    avformat
    avutil
    avfilter
    swresample
    swscale
    postproc
    avdevice
    pthreadVC2
    comctl32
    user32
    gdi32
    comdlg32
    ole32
    oleaut32
    uuid
    gdiplus
    winmm
    dbghelp
)

# 设置包含目录
target_include_directories(IJKPlayer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/demo/TestDemo
    ${CMAKE_SOURCE_DIR}/third_party/duilib
    ${CMAKE_SOURCE_DIR}/third_party/duilib/Control
    ${CMAKE_SOURCE_DIR}/third_party/duilib/Core
    ${CMAKE_SOURCE_DIR}/third_party/duilib/Layout
    ${CMAKE_SOURCE_DIR}/third_party/duilib/Utils
)

# SDL: 使用自定义 WinMain，不链接 SDL2main，并禁止 SDL 重定义 main()
target_compile_definitions(IJKPlayer PRIVATE SDL_MAIN_HANDLED)

# 设置编译选项
target_compile_options(IJKPlayer PRIVATE
    /W3
    /utf-8
)

# 禁用SAFESEH检查（FFmpeg库不支持）
target_link_options(IJKPlayer PRIVATE
    /SAFESEH:NO
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(IJKPlayer PRIVATE DEBUG)
else()
    target_compile_definitions(IJKPlayer PRIVATE NDEBUG)
endif()

# 复制DLL文件到输出目录
if(PLATFORM_ARCH STREQUAL "x64")
    set(DLL_PLATFORM_DIR "x64")
else()
    set(DLL_PLATFORM_DIR "Win32")
endif()

add_custom_command(TARGET IJKPlayer POST_BUILD
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

# 复制资源文件
add_custom_command(TARGET IJKPlayer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/res"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/res"
    COMMENT "Copying resource files"
)

