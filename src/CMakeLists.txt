# ijkwin静态库
add_library(ijkwin STATIC
    # ijkplayer核心
    ijkplayer/ff_cmdutils.c
    ijkplayer/ff_ffpipeline.c
    ijkplayer/ff_ffpipenode.c
    ijkplayer/ff_ffplay.c
    ijkplayer/ijk_ffplay_decoder.c
    ijkplayer/ijkmeta.c
    ijkplayer/ijkplayer.c
    
    # ijkplayer ijkavformat
    ijkplayer/ijkavformat/allformats.c
    ijkplayer/ijkavformat/ijkasync.c
    ijkplayer/ijkavformat/ijklivehook.c
    ijkplayer/ijkavformat/ijklongurl.c
    ijkplayer/ijkavformat/ijkmediadatasource.c
    ijkplayer/ijkavformat/ijksegment.c
    ijkplayer/ijkavformat/ijkurlhook.c
    
    # ijkplayer pipeline
    ijkplayer/pipeline/ffpipeline_ffplay.c
    ijkplayer/pipeline/ffpipenode_ffplay_vdec.c
    ijkplayer/pipeline/ffpipeline_win.c
    ijkplayer/pipeline/ffpipeline_win.c
    
    # ijksdl核心
    ijksdl/ijksdl_aout.c
    ijksdl/ijksdl_audio.c
    ijksdl/ijksdl_error.c
    ijksdl/ijksdl_mutex.c
    ijksdl/ijksdl_stdinc.c
    ijksdl/ijksdl_thread.c
    ijksdl/ijksdl_timer.c
    ijksdl/ijksdl_vout.c
    
    # ijksdl Windows特定
    ijksdl/ijksdl_aout_mixer.c
    ijksdl/ijksdl_aout_win_audio_directsound.c
    ijksdl/ijksdl_vout_win_ffmpeg.c
    ijksdl/win_directsound.c
    
    # ijksdl ffmpeg
    ijksdl/ffmpeg/abi_all/image_convert.c
    ijksdl/ffmpeg/ijksdl_vout_overlay_ffmpeg.c
    
    # global工具
    global/src/application.c
    global/src/Counter.c
    global/src/CpuInfo.c
    global/src/ifaddrs.c
    global/src/LoadAvg.c
    global/src/MemInfo.c
    global/src/network.c
    global/src/ptw32_MCS_lock.c
    global/src/resource.c
    global/src/time.c
    global/src/uio.c
    global/src/unistd.c
    global/src/UpTime.c
    global/src/Util.c
    global/src/utils.c
    global/src/win-pthread.c
    
    # dxva2硬件加速
    dxva2/ffmpeg_dxva2.c
)

# 设置目标属性
set_target_properties(ijkwin PROPERTIES
    OUTPUT_NAME "ijkwin"
    PDB_OUTPUT_DIRECTORY "${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}"
)

# 链接第三方库
target_link_libraries(ijkwin
    # FFmpeg库
    avcodec
    avformat
    avutil
    avfilter
    swresample
    swscale
    postproc
    avdevice
    
    # SDL2库
    SDL2
    SDL2main
    
    # pthread-win32
    pthreadVC2
    
    # iLog3
    iLog3
    
    # Windows系统库
    dsound
    ws2_32
    winmm
)

# 设置包含目录
target_include_directories(ijkwin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ijkplayer
    ${CMAKE_CURRENT_SOURCE_DIR}/ijkplayer/pipeline
    ${CMAKE_CURRENT_SOURCE_DIR}/ijksdl
    ${CMAKE_CURRENT_SOURCE_DIR}/global/include
    ${CMAKE_CURRENT_SOURCE_DIR}/dxva2
)

# 设置编译选项
target_compile_options(ijkwin PRIVATE
    /W3
    /wd4996  # 禁用deprecated警告
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(ijkwin PRIVATE DEBUG)
else()
    target_compile_definitions(ijkwin PRIVATE NDEBUG)
    target_compile_options(ijkwin PRIVATE
        /O2
        /GL
    )
    target_link_options(ijkwin PRIVATE
        /LTCG
    )
endif()

# 禁用SAFESEH检查（FFmpeg库不支持）
target_link_options(ijkwin PRIVATE
    /SAFESEH:NO
)

