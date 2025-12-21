#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include <string>
#include <functional>

extern "C" {
#include "ijkplayer/ijk_ffplay_decoder.h"
}

// 播放器状态回调
typedef std::function<void(IjkMsgState, int, int)> StateCallback;
// 视频帧回调
typedef std::function<void(IjkVideoFrame*)> VideoFrameCallback;

class PlayerController {
public:
    PlayerController();
    ~PlayerController();

    // 初始化/释放
    bool Initialize();
    void Release();

    // 播放控制
    bool OpenFile(const std::string& filePath);
    bool Prepare();
    bool Start();
    bool Pause();
    bool Stop();
    bool SeekTo(long positionMs);
    
    // 状态查询
    bool IsPlaying() const;
    long GetCurrentPosition() const;
    long GetDuration() const;
    float GetVolume() const;
    bool SetVolume(float volume);

    // 设置回调
    void SetStateCallback(StateCallback callback);
    void SetVideoFrameCallback(VideoFrameCallback callback);

    // 获取媒体信息
    bool GetVideoCodecInfo(std::string& info);
    bool GetAudioCodecInfo(std::string& info);
    bool GetMediaMeta(IjkMetadata* metadata);

private:
    IjkFfplayDecoder* m_decoder;
    StateCallback m_stateCallback;
    VideoFrameCallback m_videoFrameCallback;
    bool m_initialized;
    bool m_playing;

    // 静态回调函数
    static void StaticIJKLogCallback(void* opaque, int level, const char* szFmt, va_list varg);
    static void StaticVideoCallback(void* opaque, IjkVideoFrame* frame);
    static void StaticStateCallback(void* opaque, IjkMsgState state, int arg1, int arg2);
};

#endif // PLAYER_CONTROLLER_H


