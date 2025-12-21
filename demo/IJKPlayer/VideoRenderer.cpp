#include "VideoRenderer.h"
#include "logging.h"
#include <cstring>
#include <algorithm>

VideoRenderer::VideoRenderer()
    : m_hwnd(nullptr)
    , m_sdlWindow(nullptr)
    , m_sdlRenderer(nullptr)
    , m_sdlTexture(nullptr)
    , m_initialized(false)
    , m_fullscreen(false)
    , m_videoWidth(0)
    , m_videoHeight(0)
    , m_pixelFormat(PIX_FMT_NONE)
    , m_lastWndW(0)
    , m_lastWndH(0)
    , m_frameW(0)
    , m_frameH(0)
    , m_frameFormat(PIX_FMT_NONE)
    , m_frameLinesize0(0)
    , m_frameLinesize1(0)
    , m_frameLinesize2(0)
    , m_hasPendingFrame(false)
{
    memset(&m_sdlRect, 0, sizeof(m_sdlRect));
}

VideoRenderer::~VideoRenderer()
{
    Release();
}

bool VideoRenderer::Initialize(HWND hwnd)
{
    if (m_initialized) {
        Release();
    }

    if (!hwnd || !IsWindow(hwnd)) {
        Log::Error("Invalid HWND for VideoRenderer");
        return false;
    }

    m_hwnd = hwnd;

    // 初始化SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER) < 0) {
        Log::Error("SDL_Init failed: %s", SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    // 从HWND创建SDL窗口（renderer/texture 延迟到首帧时创建，避免窗口未就绪导致失败）
    m_sdlWindow = SDL_CreateWindowFrom((void*)m_hwnd);
    if (!m_sdlWindow) {
        Log::Error("SDL_CreateWindowFrom failed: %s", SDL_GetError());
        SDL_Quit();
        return false;
    }

    m_initialized = true;
    Log::Info("VideoRenderer initialized with HWND: %p", m_hwnd);
    return true;
}

void VideoRenderer::Release()
{
    CleanupSDL();

    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        m_frameBuffer.clear();
        m_hasPendingFrame = false;
        m_frameW = 0;
        m_frameH = 0;
        m_frameFormat = PIX_FMT_NONE;
        m_frameLinesize0 = 0;
        m_frameLinesize1 = 0;
        m_frameLinesize2 = 0;
    }

    if (m_initialized) {
        SDL_Quit();
        m_initialized = false;
    }

    m_hwnd = nullptr;
    m_videoWidth = 0;
    m_videoHeight = 0;
    m_pixelFormat = PIX_FMT_NONE;
}

void VideoRenderer::SubmitFrame(IjkVideoFrame* frame)
{
    if (!m_initialized || !frame) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_frameW = frame->w;
    m_frameH = frame->h;
    m_frameFormat = frame->format;
    m_frameLinesize0 = frame->linesize[0];
    m_frameLinesize1 = frame->linesize[1];
    m_frameLinesize2 = frame->linesize[2];

    if (frame->format == PIX_FMT_YUV420P) {
        const size_t ySize = (size_t)m_frameH * (size_t)m_frameLinesize0;
        const size_t uSize = (size_t)(m_frameH / 2) * (size_t)m_frameLinesize1;
        const size_t vSize = (size_t)(m_frameH / 2) * (size_t)m_frameLinesize2;
        m_frameBuffer.resize(ySize + uSize + vSize);
        memcpy(m_frameBuffer.data(), frame->data[0], ySize);
        memcpy(m_frameBuffer.data() + ySize, frame->data[1], uSize);
        memcpy(m_frameBuffer.data() + ySize + uSize, frame->data[2], vSize);
        m_hasPendingFrame = true;
    } else if (frame->format == PIX_FMT_NV12) {
        const size_t ySize = (size_t)m_frameH * (size_t)m_frameLinesize0;
        const size_t uvSize = (size_t)(m_frameH / 2) * (size_t)m_frameLinesize1;
        m_frameBuffer.resize(ySize + uvSize);
        memcpy(m_frameBuffer.data(), frame->data[0], ySize);
        memcpy(m_frameBuffer.data() + ySize, frame->data[1], uvSize);
        m_hasPendingFrame = true;
    } else {
        m_hasPendingFrame = false;
    }
}

void VideoRenderer::RenderPending()
{
    if (!m_initialized || !m_sdlWindow) {
        return;
    }

    // 关键：我们没有 SDL 的事件循环，手动泵一下事件让 SDL 同步窗口状态（尤其是 resize/maximize）
    SDL_PumpEvents();

    int w = 0;
    int h = 0;
    IjkPixelFormat fmt = PIX_FMT_NONE;
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (!m_hasPendingFrame) {
            return;
        }
        w = m_frameW;
        h = m_frameH;
        fmt = m_frameFormat;
    }

    if (m_videoWidth != w || m_videoHeight != h || m_pixelFormat != fmt) {
        SetupSDL(w, h, fmt);
    }

    if (!m_sdlRenderer || !m_sdlTexture) {
        return;
    }

    UpdateTextureFromBuffer();
    UpdateDestinationRect();

    // 确保 viewport 不残留导致裁剪
    SDL_RenderSetViewport(m_sdlRenderer, NULL);
    SDL_RenderClear(m_sdlRenderer);
    SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, NULL, &m_sdlRect);
    SDL_RenderPresent(m_sdlRenderer);
}

void VideoRenderer::SetFullscreen(bool fullscreen)
{
    if (!m_sdlWindow) {
        return;
    }

    m_fullscreen = fullscreen;
    SDL_SetWindowFullscreen(m_sdlWindow, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

void VideoRenderer::CleanupSDL()
{
    CleanupTexture();

    if (m_sdlRenderer) { SDL_DestroyRenderer(m_sdlRenderer); m_sdlRenderer = nullptr; }

    if (m_sdlWindow) {
        SDL_DestroyWindow(m_sdlWindow);
        m_sdlWindow = nullptr;
    }
}

void VideoRenderer::CleanupTexture()
{
    if (m_sdlTexture) {
        SDL_DestroyTexture(m_sdlTexture);
        m_sdlTexture = nullptr;
    }
}

void VideoRenderer::SetupSDL(int width, int height, IjkPixelFormat format)
{
    // 不能销毁/重建 SDL_Window（它来自 HWND），否则 SDL_CreateRenderer 可能失败
    CleanupTexture();

    m_videoWidth = width;
    m_videoHeight = height;
    m_pixelFormat = format;

    if (!m_sdlWindow) {
        Log::Error("SDL window is null, cannot setup renderer/texture");
        return;
    }

    int screen_w = width;
    int screen_h = height;

    // 创建 renderer（首帧创建，尽量贴近 MediaPlayer 示例）
    if (!m_sdlRenderer) {
        m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_ACCELERATED);
        if (!m_sdlRenderer) {
            Log::Warn("SDL_CreateRenderer accelerated failed: %s, fallback to software", SDL_GetError());
            m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, 0);
        }
        if (!m_sdlRenderer) {
            Log::Error("Failed to create SDL renderer: %s", SDL_GetError());
            return;
        }
    }

    // 不使用 logical size（否则 dstRect 会在“逻辑坐标系”里计算，导致缩放/裁剪异常）
    SDL_RenderSetLogicalSize(m_sdlRenderer, 0, 0);

    // 根据格式创建纹理
    if (format == PIX_FMT_YUV420P) {
        m_sdlTexture = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_IYUV,
            SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
    } else if (format == PIX_FMT_NV12) {
        m_sdlTexture = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_NV12,
            SDL_TEXTUREACCESS_STREAMING, screen_w, screen_h);
    } else {
        Log::Warn("Unsupported pixel format: %d", format);
        return;
    }

    if (!m_sdlTexture) {
        Log::Error("Failed to create SDL texture: %s", SDL_GetError());
        return;
    }

    m_sdlRect.x = 0;
    m_sdlRect.y = 0;
    m_sdlRect.w = screen_w;
    m_sdlRect.h = screen_h;
    m_lastWndW = 0;
    m_lastWndH = 0;

    Log::Info("SDL setup: %dx%d, format=%d", screen_w, screen_h, format);
}

void VideoRenderer::UpdateDestinationRect()
{
    if (!m_sdlWindow || !m_sdlRenderer || m_videoWidth <= 0 || m_videoHeight <= 0) {
        return;
    }

    // 关键：不要依赖 SDL 的窗口尺寸（我们没有 SDL_PollEvent/PumpEvents，SDL 可能拿不到最新尺寸）
    // 直接用 WinAPI 获取真实 HWND client size
    RECT rcClient{};
    if (!m_hwnd || !::IsWindow(m_hwnd) || !::GetClientRect(m_hwnd, &rcClient)) {
        return;
    }
    int wndW = rcClient.right - rcClient.left;
    int wndH = rcClient.bottom - rcClient.top;
    if (wndW <= 0 || wndH <= 0) {
        return;
    }

    // 让 SDL 同步窗口尺寸（我们是 CreateWindowFrom 场景，SDL 内部不一定能及时感知 HWND resize）
    SDL_SetWindowSize(m_sdlWindow, wndW, wndH);

    // 如果 renderer 的输出仍然是旧尺寸，会导致 RenderCopy 被裁剪（只显示一部分）
    // 这里检测 mismatch，并重建 renderer/texture（在 UI 线程调用，安全）
    ResetRendererForResizeIfNeeded(wndW, wndH);

    if (wndW == m_lastWndW && wndH == m_lastWndH) {
        return;
    }
    m_lastWndW = wndW;
    m_lastWndH = wndH;

    // 等比缩放 + 居中（letterbox）
    const double srcW = (double)m_videoWidth;
    const double srcH = (double)m_videoHeight;
    const double dstW = (double)wndW;
    const double dstH = (double)wndH;

    const double srcAspect = srcW / srcH;
    const double dstAspect = dstW / dstH;

    int outW = wndW;
    int outH = wndH;
    if (dstAspect > srcAspect) {
        // 窗口更宽，以高度为准
        outH = wndH;
        outW = (int)(outH * srcAspect);
    } else {
        // 窗口更高，以宽度为准
        outW = wndW;
        outH = (int)(outW / srcAspect);
    }

    m_sdlRect.w = outW;
    m_sdlRect.h = outH;
    m_sdlRect.x = (wndW - outW) / 2;
    m_sdlRect.y = (wndH - outH) / 2;

    Log::Info("RenderRect: wnd=%dx%d video=%dx%d dst=%d,%d,%d,%d",
        wndW, wndH, m_videoWidth, m_videoHeight, m_sdlRect.x, m_sdlRect.y, m_sdlRect.w, m_sdlRect.h);
}

void VideoRenderer::ResetRendererForResizeIfNeeded(int wndW, int wndH)
{
    if (!m_sdlRenderer) return;

    int outW = 0, outH = 0;
    if (SDL_GetRendererOutputSize(m_sdlRenderer, &outW, &outH) != 0) {
        return;
    }
    // 允许小的误差（边框/四舍五入）
    if (abs(outW - wndW) <= 2 && abs(outH - wndH) <= 2) {
        return;
    }

    Log::Warn("RendererOutputSize mismatch: renderer=%dx%d hwnd=%dx%d, recreate renderer/texture", outW, outH, wndW, wndH);

    // 先销毁 texture，再销毁 renderer，然后重建（texture 会在 SetupSDL 里创建）
    CleanupTexture();
    if (m_sdlRenderer) {
        SDL_DestroyRenderer(m_sdlRenderer);
        m_sdlRenderer = nullptr;
    }
    m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, SDL_RENDERER_ACCELERATED);
    if (!m_sdlRenderer) {
        Log::Warn("SDL_CreateRenderer accelerated failed after resize: %s, fallback to software", SDL_GetError());
        m_sdlRenderer = SDL_CreateRenderer(m_sdlWindow, -1, 0);
    }
    if (!m_sdlRenderer) {
        Log::Error("Failed to recreate SDL renderer: %s", SDL_GetError());
        return;
    }

    // 重新创建 texture（沿用当前视频尺寸/格式）
    SetupSDL(m_videoWidth, m_videoHeight, m_pixelFormat);
}

void VideoRenderer::UpdateTextureFromBuffer()
{
    if (!m_sdlTexture) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_frameMutex);
    if (!m_hasPendingFrame || m_frameBuffer.empty()) {
        return;
    }

    if (m_frameFormat == PIX_FMT_YUV420P) {
        const size_t ySize = (size_t)m_frameH * (size_t)m_frameLinesize0;
        const size_t uSize = (size_t)(m_frameH / 2) * (size_t)m_frameLinesize1;
        const unsigned char* y = m_frameBuffer.data();
        const unsigned char* u = m_frameBuffer.data() + ySize;
        const unsigned char* v = m_frameBuffer.data() + ySize + uSize;
        SDL_UpdateYUVTexture(m_sdlTexture, NULL, y, m_frameLinesize0, u, m_frameLinesize1, v, m_frameLinesize2);
    } else if (m_frameFormat == PIX_FMT_NV12) {
        SDL_UpdateTexture(m_sdlTexture, NULL, m_frameBuffer.data(), m_frameLinesize0);
    }
}


