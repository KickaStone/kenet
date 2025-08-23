# Logger 使用说明

## 快速开始

### 智能自动管理（推荐）

最简单的使用方式，无需任何配置和手动管理：

```cpp
#include "logger/log.h"

int main() {
    // 直接使用日志，logger会自动启动和停止
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    
    // 程序退出时logger会自动停止
    return 0;
}
```

### 手动管理（如果需要自定义配置）

如果需要自定义配置，可以手动管理生命周期：

```cpp
#include "logger/log.h"

int main() {
    // 使用默认配置启动logger
    AsyncLogger<>::instance().start();
    
    // 直接使用日志
    LOG_DEBUG("Debug message");
    LOG_INFO("Info message");
    LOG_WARN("Warning message");
    LOG_ERROR("Error message");
    
    // 停止logger
    AsyncLogger<>::instance().stop();
    
    return 0;
}
```

### 默认配置说明

默认配置的特点：
- **只启用控制台输出**：文件输出被禁用（`file_level = Level::Off`）
- **控制台级别为Debug**：可以看到所有级别的日志
- **启用颜色输出**：控制台日志带有颜色标识
- **刷新间隔**：100ms
- **刷新字节数**：128KB

### 自定义配置

如果需要自定义配置，可以传递Config对象：

```cpp
#include "logger/log.h"

int main() {
    Config cfg;
    cfg.path = "myapp.log";
    cfg.file_level = Level::Info;      // 文件输出从Info开始
    cfg.console_level = Level::Warn;   // 控制台从Warn开始
    cfg.flush_bytes = 64 << 10;        // 64KB
    cfg.flush_interval = std::chrono::milliseconds(200);
    cfg.console_enable_color = true;
    
    AsyncLogger<>::instance().start(cfg);
    
    // 使用日志...
    
    AsyncLogger<>::instance().stop();
    return 0;
}
```

## 日志级别

- `Level::Trace` - 最详细的调试信息
- `Level::Debug` - 调试信息
- `Level::Info` - 一般信息
- `Level::Warn` - 警告信息
- `Level::Error` - 错误信息
- `Level::Fatal` - 致命错误
- `Level::Off` - 禁用输出

## 使用方式

### 宏方式（推荐）
```cpp
LOG_DEBUG("Debug: %d", value);
LOG_INFO("Info: %s", message);
LOG_WARN("Warning: %.2f", percentage);
LOG_ERROR("Error: %s", error_msg);
```

### 实例方法
```cpp
auto& logger = AsyncLogger<>::instance();
logger.debug("Debug message");
logger.info("Info message");
logger.warn("Warning message");
logger.error("Error message");
```

## 配置选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `path` | "app.log" | 日志文件路径 |
| `file_level` | Level::Off | 文件输出级别（默认禁用） |
| `console_level` | Level::Debug | 控制台输出级别 |
| `flush_bytes` | 128KB | 刷新字节数阈值 |
| `flush_interval` | 100ms | 刷新时间间隔 |
| `console_enable_color` | true | 是否启用控制台颜色 |
| `rotate_bytes` | 128MB | 文件轮转大小 |
| `fsync_on_flush` | false | 是否在刷新时同步到磁盘 |
