#include "PlayerController.h"
#include "logging.h"
#include <cstring>
#include <cstdarg>

PlayerController::PlayerController()
    : m_decoder(nullptr)
    , m_initialized(false)
    , m_playing(false)
{
}

PlayerController::~PlayerController()
{
    Release();
}

bool PlayerController::Initialize()
{
    if (m_initialized) {
        return true;
    }

    // 初始化ijkplayer
    int ret = ijkFfplayDecoder_init();
    if (ret != 0) {
        Log::Error("Failed to initialize ijkplayer: %d", ret);
        return false;
    }

    // 打开ijk内部日志，便于定位“无法播放”的根因
    ijkFfplayDecoder_setLogLevel(k_IJK_LOG_DEBUG);
    ijkFfplayDecoder_setLogCallback(StaticIJKLogCallback);

    // 创建解码器
    m_decoder = ijkFfplayDecoder_create();
    if (!m_decoder) {
        Log::Error("Failed to create ijkplayer decoder");
        ijkFfplayDecoder_uninit();
        return false;
    }

    // 明确设置为软件/默认解码器（避免未初始化硬解名称导致失败）
    ijkFfplayDecoder_setHwDecoderName(m_decoder, NULL);

    // 设置回调
    IjkFfplayDecoderCallBack callback;
    callback.func_get_frame = StaticVideoCallback;
    callback.func_state_change = StaticStateCallback;
    ijkFfplayDecoder_setDecoderCallBack(m_decoder, this, &callback);

    m_initialized = true;
    Log::Info("PlayerController initialized");
    return true;
}

void PlayerController::Release()
{
    if (m_decoder) {
        // 关闭阶段先断开解码器回调，避免 release 过程中/之后仍然回调到已释放的 PlayerController
        IjkFfplayDecoderCallBack cb{};
        cb.func_get_frame = nullptr;
        cb.func_state_change = nullptr;
        ijkFfplayDecoder_setDecoderCallBack(m_decoder, nullptr, &cb);

        // 确保 stop 一次，降低后台线程残留概率
        ijkFfplayDecoder_stop(m_decoder);

        ijkFfplayDecoder_release(m_decoder);
        m_decoder = nullptr;
    }

    if (m_initialized) {
        ijkFfplayDecoder_uninit();
        m_initialized = false;
    }

    m_playing = false;
}

bool PlayerController::OpenFile(const std::string& filePath)
{
    if (!m_initialized || !m_decoder) {
        Log::Error("PlayerController not initialized");
        return false;
    }

    int ret = ijkFfplayDecoder_setDataSource(m_decoder, filePath.c_str());
    if (ret != 0) {
        Log::Error("Failed to set data source: %s", filePath.c_str());
        return false;
    }

    Log::Info("Opened file: %s", filePath.c_str());
    return true;
}

bool PlayerController::Prepare()
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    int ret = ijkFfplayDecoder_prepare(m_decoder);
    if (ret != 0) {
        Log::Error("Failed to prepare decoder: %d", ret);
        return false;
    }

    return true;
}

bool PlayerController::Start()
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    int ret = ijkFfplayDecoder_start(m_decoder);
    if (ret != 0) {
        Log::Error("Failed to start decoder: %d", ret);
        return false;
    }

    m_playing = true;
    return true;
}

bool PlayerController::Pause()
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    int ret = ijkFfplayDecoder_pause(m_decoder);
    if (ret != 0) {
        Log::Error("Failed to pause decoder: %d", ret);
        return false;
    }

    m_playing = false;
    return true;
}

bool PlayerController::Stop()
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    int ret = ijkFfplayDecoder_stop(m_decoder);
    if (ret != 0) {
        Log::Error("Failed to stop decoder: %d", ret);
        return false;
    }

    m_playing = false;
    return true;
}

bool PlayerController::SeekTo(long positionMs)
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    int ret = ijkFfplayDecoder_seekTo(m_decoder, positionMs);
    if (ret != 0) {
        Log::Error("Failed to seek: %d", ret);
        return false;
    }

    return true;
}

bool PlayerController::IsPlaying() const
{
    if (!m_initialized || !m_decoder) {
        return false;
    }
    return m_playing && ijkFfplayDecoder_isPlaying(m_decoder);
}

long PlayerController::GetCurrentPosition() const
{
    if (!m_initialized || !m_decoder) {
        return 0;
    }
    return ijkFfplayDecoder_getCurrentPosition(m_decoder);
}

long PlayerController::GetDuration() const
{
    if (!m_initialized || !m_decoder) {
        return 0;
    }
    return ijkFfplayDecoder_getDuration(m_decoder);
}

float PlayerController::GetVolume() const
{
    if (!m_initialized || !m_decoder) {
        return 0.0f;
    }
    return ijkFfplayDecoder_getVolume(m_decoder);
}

bool PlayerController::SetVolume(float volume)
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    // SDK 的示例使用 0~100（参考 ijkDemo/MediaPlayer）
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 100.0f) volume = 100.0f;

    int ret = ijkFfplayDecoder_setVolume(m_decoder, volume);
    if (ret != 0) {
        Log::Error("Failed to set volume: %d", ret);
        return false;
    }

    return true;
}

void PlayerController::StaticIJKLogCallback(void* /*opaque*/, int level, const char* szFmt, va_list varg)
{
    char line[1024] = { 0 };
    vsnprintf(line, sizeof(line), szFmt, varg);

    switch (level) {
    case k_IJK_LOG_DEBUG:
        Log::Debug("%s", line);
        break;
    case k_IJK_LOG_INFO:
        Log::Info("%s", line);
        break;
    case k_IJK_LOG_WARN:
        Log::Warn("%s", line);
        break;
    case k_IJK_LOG_ERROR:
        Log::Error("%s", line);
        break;
    case k_IJK_LOG_FATAL:
        Log::Fatal("%s", line);
        break;
    default:
        Log::Info("%s", line);
        break;
    }
}

void PlayerController::SetStateCallback(StateCallback callback)
{
    m_stateCallback = callback;
}

void PlayerController::SetVideoFrameCallback(VideoFrameCallback callback)
{
    m_videoFrameCallback = callback;
}

bool PlayerController::GetVideoCodecInfo(std::string& info)
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    char* codecInfo = nullptr;
    int ret = ijkFfplayDecoder_getVideoCodecInfo(m_decoder, &codecInfo);
    if (ret == 0 && codecInfo) {
        info = codecInfo;
        return true;
    }
    return false;
}

bool PlayerController::GetAudioCodecInfo(std::string& info)
{
    if (!m_initialized || !m_decoder) {
        return false;
    }

    char* codecInfo = nullptr;
    int ret = ijkFfplayDecoder_getAudioCodecInfo(m_decoder, &codecInfo);
    if (ret == 0 && codecInfo) {
        info = codecInfo;
        return true;
    }
    return false;
}

bool PlayerController::GetMediaMeta(IjkMetadata* metadata)
{
    if (!m_initialized || !m_decoder || !metadata) {
        return false;
    }

    int ret = ijkFfplayDecoder_getMediaMeta(m_decoder, metadata);
    return ret == 0;
}

void PlayerController::StaticVideoCallback(void* opaque, IjkVideoFrame* frame)
{
    PlayerController* controller = static_cast<PlayerController*>(opaque);
    if (controller && controller->m_videoFrameCallback) {
        controller->m_videoFrameCallback(frame);
    }
}

void PlayerController::StaticStateCallback(void* opaque, IjkMsgState state, int arg1, int arg2)
{
    PlayerController* controller = static_cast<PlayerController*>(opaque);
    if (controller && controller->m_stateCallback) {
        controller->m_stateCallback(state, arg1, arg2);
    }
}


