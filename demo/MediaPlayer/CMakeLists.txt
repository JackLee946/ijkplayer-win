# MediaPlayer - MFC GUI示例程序
# 注意：MFC程序需要特殊配置，这里提供基本框架

# 检查MFC支持
if(MSVC)
    # 设置MFC
    set(CMAKE_MFC_FLAG 2)  # 使用共享MFC DLL
    
    add_executable(MediaPlayer WIN32
        MediaPlayer.cpp
        MediaPlayerDlg.cpp
        logging.cpp
        pch.cpp
        MediaPlayer.rc
        res/MediaPlayer.ico
        res/MediaPlayer.rc2
    )
    
    # 设置预编译头
    target_precompile_headers(MediaPlayer PRIVATE
        pch.h
    )
    
    # 链接ijkwin库和MFC库
    target_link_libraries(MediaPlayer
        ijkwin
    )
    
    # 设置包含目录
    target_include_directories(MediaPlayer PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
    )
    
    # 设置编译选项
    target_compile_options(MediaPlayer PRIVATE
        /W3
        /EHsc
    )
    
    # 禁用SAFESEH检查（FFmpeg库不支持）
    target_link_options(MediaPlayer PRIVATE
        /SAFESEH:NO
    )
    
    # 设置资源文件
    set_source_files_properties(MediaPlayer.rc PROPERTIES
        LANGUAGE RC
    )
    
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(MediaPlayer PRIVATE DEBUG _DEBUG)
    else()
        target_compile_definitions(MediaPlayer PRIVATE NDEBUG)
    endif()
    
    # 复制DLL文件到输出目录
    if(PLATFORM_ARCH STREQUAL "x64")
        set(DLL_PLATFORM_DIR "x64")
    else()
        set(DLL_PLATFORM_DIR "Win32")
    endif()
    
    add_custom_command(TARGET MediaPlayer POST_BUILD
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
else()
    message(WARNING "MediaPlayer requires MSVC compiler for MFC support")
endif()

