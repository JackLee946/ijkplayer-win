#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include <windows.h>
#include <memory>
#include <mutex>
#include <vector>

extern "C" {
#include "SDL.h"
#include "ijkplayer/ijk_frame.h"
}

class VideoRenderer {
public:
    VideoRenderer();
    ~VideoRenderer();

    // 初始化/释放
    bool Initialize(HWND hwnd);
    void Release();

    // 解码线程提交帧（深拷贝），UI线程渲染
    void SubmitFrame(IjkVideoFrame* frame);
    void RenderPending();

    // 全屏控制
    void SetFullscreen(bool fullscreen);
    bool IsFullscreen() const { return m_fullscreen; }

    // 获取视频尺寸
    int GetVideoWidth() const { return m_videoWidth; }
    int GetVideoHeight() const { return m_videoHeight; }

private:
    HWND m_hwnd;
    SDL_Window* m_sdlWindow;
    SDL_Renderer* m_sdlRenderer;
    SDL_Texture* m_sdlTexture;
    SDL_Rect m_sdlRect;
    
    bool m_initialized;
    bool m_fullscreen;
    int m_videoWidth;
    int m_videoHeight;
    IjkPixelFormat m_pixelFormat;
    int m_lastWndW;
    int m_lastWndH;
    
    // 帧缓存（深拷贝），避免跨线程直接使用 overlay 内存造成堆损坏
    std::mutex m_frameMutex;
    std::vector<unsigned char> m_frameBuffer;
    int m_frameW;
    int m_frameH;
    IjkPixelFormat m_frameFormat;
    int m_frameLinesize0;
    int m_frameLinesize1;
    int m_frameLinesize2;
    bool m_hasPendingFrame;

    void CleanupTexture();
    void CleanupSDL();
    void ResetRendererForResizeIfNeeded(int wndW, int wndH);
    void SetupSDL(int width, int height, IjkPixelFormat format);
    void UpdateTextureFromBuffer();
    void UpdateDestinationRect();
};

#endif // VIDEO_RENDERER_H


