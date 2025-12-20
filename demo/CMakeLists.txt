# Demo程序目录
add_subdirectory(TestDemo)
add_subdirectory(ijkDemo)

# MediaPlayer需要MFC，可选编译
if(MSVC)
    option(BUILD_MEDIAPLAYER "Build MediaPlayer MFC demo" OFF)
    if(BUILD_MEDIAPLAYER)
        add_subdirectory(MediaPlayer)
    endif()
endif()

