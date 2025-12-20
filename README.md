# IJKMediaPlayer Windows 构建说明

## 目录结构

```
.
├── CMakeLists.txt          # 主CMake构建文件
├── include/                # 公共头文件
│   ├── ijkplayer/         # ijkplayer API头文件
│   ├── ijksdl/            # ijksdl头文件
│   └── global/            # 全局工具头文件
├── src/                    # 源代码
│   ├── ijkplayer/         # ijkplayer实现
│   ├── ijksdl/            # ijksdl Windows实现
│   ├── global/            # 全局工具实现
│   └── dxva2/             # DXVA2硬件加速
├── lib/                    # 编译输出库文件
│   ├── Win32/
│   │   ├── Debug/
│   │   └── Release/
│   └── x64/
│       ├── Debug/
│       └── Release/
├── third_party/            # 第三方依赖库
│   ├── ffmpeg/            # FFmpeg库
│   ├── sdl2/              # SDL2库
│   ├── pthread-win32/     # pthread-win32库
│   └── iLog3/             # iLog3日志库
└── demo/                   # 示例程序
    ├── TestDemo/          # 控制台示例
    ├── MediaPlayer/       # MFC GUI示例
    └── ijkDemo/          # 简单示例
```

## 构建要求

- **CMake**: 3.15 或更高版本
- **编译器**: Visual Studio 2019 或更高版本
- **平台**: Windows (Win32/x64)

## 构建步骤

### 1. 使用构建脚本（推荐）

项目提供了便捷的构建脚本，会自动处理所有构建步骤：

**Windows批处理脚本：**
```bash
# Win32 Release版本（默认）
build.bat

# Win32 Debug版本
build.bat Win32 Debug

# x64 Release版本
build.bat x64 Release

# x64 Debug版本
build.bat x64 Debug
```

**PowerShell脚本：**
```powershell
# Win32 Release版本（默认）
.\build.ps1

# 指定平台和配置
.\build.ps1 -Platform Win32 -BuildType Release
.\build.ps1 -Platform x64 -BuildType Debug
```

**清理构建文件：**
```bash
# 清理build和output目录
clean.bat
# 或
.\clean.ps1
```

### 2. 使用CMake命令行构建

```bash
# 在项目根目录下创建构建目录
mkdir build
cd build

# 配置CMake（Win32平台）
cmake .. -G "Visual Studio 16 2019" -A Win32

# 或配置为x64平台
cmake .. -G "Visual Studio 16 2019" -A x64

# 编译
cmake --build . --config Release

# 或编译Debug版本
cmake --build . --config Debug
```

### 2. 使用Visual Studio 2019打开

```bash
# 在项目根目录下生成Visual Studio解决方案
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019" -A Win32

# 打开生成的解决方案文件
start IJKMediaPlayer_Win.sln
```

在Visual Studio中：
1. 选择配置（Debug/Release）
2. 选择平台（Win32/x64）
3. 生成解决方案（Build Solution）

### 3. 使用CMake GUI

1. 打开CMake GUI
2. 设置源代码目录为项目根目录
3. 设置构建目录（例如 `build`）
4. 点击 "Configure"，选择 "Visual Studio 16 2019" 和平台（Win32或x64）
5. 点击 "Generate"
6. 点击 "Open Project" 在Visual Studio中打开

## 输出文件

编译完成后，所有输出文件位于 `output` 目录：

- **库文件**: `output/ijkwin.lib`
- **可执行文件**: `output/`
  - `TestDemo.exe` - 控制台示例程序
  - `ijkDemo.exe` - 简单示例程序
  - `MediaPlayer.exe` - MFC GUI示例（如果启用）
- **依赖DLL**: `output/`
  - FFmpeg DLL文件（avcodec-57.dll, avformat-57.dll等）
  - SDL2.dll
  - pthreadVC2.dll

所有文件都在 `output` 目录的根目录下，方便直接运行。

## 第三方依赖库

项目依赖以下第三方库，已包含在 `third_party` 目录中：

- **FFmpeg**: 音视频编解码库
- **SDL2**: 跨平台多媒体库
- **pthread-win32**: Windows平台的pthread实现
- **iLog3**: 日志库

### 依赖库目录结构

每个第三方库的目录结构如下：

```
third_party/{library}/
├── include/          # 头文件
└── lib/
    ├── Win32/       # Win32平台库文件
    └── x64/         # x64平台库文件
```

## 配置选项

### 构建MediaPlayer示例

MediaPlayer是MFC应用程序，默认不构建。要启用它：

```bash
cmake .. -DBUILD_MEDIAPLAYER=ON
```

或在CMake GUI中设置 `BUILD_MEDIAPLAYER` 为 `ON`。

## 运行示例程序

### TestDemo

```bash
cd output
TestDemo.exe <video_file>
```

### ijkDemo

```bash
cd output
ijkDemo.exe <video_file>
```

所有依赖的DLL文件已经自动复制到 `output` 目录，可以直接运行。

## 常见问题

### 1. DLL文件找不到

确保以下DLL文件在可执行文件同一目录下：
- FFmpeg DLL文件（avcodec-57.dll, avformat-57.dll等）
- SDL2.dll
- pthreadVC2.dll

CMakeLists.txt已配置自动复制这些DLL文件到输出目录。

### 2. 链接错误

检查第三方库文件是否存在于 `third_party/{library}/lib/{Win32|x64}/` 目录中。

### 3. 包含路径错误

确保使用正确的include路径：
- 公共API: `#include "ijkplayer/ijk_ffplay_decoder.h"`
- 第三方库: `#include "SDL.h"`, `#include "libavformat/avformat.h"`

## 开发说明

### 添加新的源文件

1. 将源文件添加到 `src` 目录的相应子目录
2. 在 `src/CMakeLists.txt` 中添加源文件路径

### 添加新的demo

1. 在 `demo` 目录下创建新目录
2. 创建 `CMakeLists.txt`
3. 在 `demo/CMakeLists.txt` 中添加 `add_subdirectory()`

## 许可证

请参考项目根目录的LICENSE文件。

