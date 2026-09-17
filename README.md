# ANSI Samples

一组使用 ANSI 转义序列在终端里做实时图形与交互的小例子。所有渲染都以“背景色方块”为单位（每个像素 `Renderer` 会输出两个空格），支持真彩色（24-bit）。

## 环境要求

- CMake 3.16+
- 支持 C++17 的编译器（MSVC / GCC / Clang）
- 一个支持真彩色 ANSI 的终端（Windows Terminal、大部分 Linux/macOS 终端）

## 构建

```sh
cmake -S . -B build
cmake --build build
```

也可以只构建某个例子：

```sh
cmake --build build --target 03_cube
```

可执行文件输出在 `build/samples/`。Windows 下为 `*.exe`，运行例如：

```sh
build\samples\03_cube.exe
```

`04_snake` 与 `08_fluid` 支持按键交互（`Q` / `Esc` 退出），其余动画样例按 `Ctrl+C` 退出。

## 目录结构

```
include/          头文件（库本体）
externals/        第三方库：stb_image、tiny_obj_loader
assets/           示例资源（image.png、teapot.obj）
samples/          各个示例程序
```

### include

| 文件 | 说明 |
| --- | --- |
| `color.hpp` | `Color` 结构体、`RESET`、后台色转义输出辅助函数 |
| `math.hpp` | `Vec2` / `Vec3`、`dot` / `cross` / `normalize` / `rotate` / `edge` 等数学工具 |
| `terminal.hpp` | 光标定位、隐藏/显示光标、清屏，以及 RAII 的 `CursorGuard` |
| `renderer.hpp` | `Renderer`：像素帧缓冲、`set_pixel`、带脏像素 diff 的 `present` |
| `raster.hpp` | `fill_triangle`：基于重心坐标、可做顶点颜色插值的三角形光栅化 |
| `input.hpp` | `Key` 枚举、`Input`：非阻塞按键读取（Windows `conio` / POSIX `termios`）、`sleep_ms` |
| `ansi.hpp` | 聚合头，包含以上全部 |

## 示例

| # | 文件 | 说明 |
| --- | --- | --- |
| 01 | `01_hello.cpp` | 最小示例。不依赖库，直接输出一张 RGB 渐变图，并展示最基础的 ANSI 后台色写法。 |
| 02 | `02_triangle.cpp` | 旋转的三角形，使用重心坐标对三个顶点颜色做插值。80x40 网格。 |
| 03 | `03_cube.cpp` | 旋转立方体。透视投影、背面剔除、画家算法排序、按法线的简单光照。160x80 网格。 |
| 04 | `04_snake.cpp` | 可交互的贪吃蛇游戏。60x30 网格，WASD / 方向键控制，吃食物变长，撞墙或撞自己结束，结束后 `R` 重开、`Q` 退出。 |
| 05 | `05_image.cpp` | 用 stb_image 加载 `assets/image.png`，盒式降采样后按终端宽高比缩放显示（最大 140x70）。 |
| 06 | `06_obj.cpp` | 用 tiny_obj_loader 加载 `assets/teapot.obj`，自动计算平滑法线，使用透视校正插值与 z-buffer，逐像素 Blinn-Phong 着色。同一场景并排渲染 2 个茶壶（各自相位不同），每个茶壶使用不同材质（石膏/铜交替）。 |
| 07 | `07_raymarch.cpp` | Raymarching 实时场景：模型在立方体与球体之间循环变形并自转，镜面棋盘地面，含软阴影、环境光遮蔽、Fresnel 高光与一次反射。相机自动环绕。120x60 网格。 |
| 08 | `08_fluid.cpp` | 基于欧拉网格法的 2D 稳定流体模拟（MacCormack 平流 + 压力投影 + 涡量约束）。左侧喷口以恒定参数向右喷射流体，击中圆形障碍产生涡流脱落，右侧为开放出口（压力 Dirichlet + 零梯度速度）。可用速度热力图或涡量发散色标显示（Space 切换）。160x80 网格。 |

`05_image` 和 `06_obj` 的资源路径默认由 CMake 通过 `ASSET_DIR` 宏注入，也可以在命令行覆盖：

```sh
build\samples\05_image.exe path\to\image.png
build\samples\06_obj.exe path\to\model.obj
```

## 第三方库

- [stb_image](https://github.com/nothings/stb) — 图片解码，位于 `externals/stb`
- [tiny_obj_loader](https://github.com/tinyobjloader/tinyobjloader) — OBJ 模型加载，位于 `externals/tiny_obj_loader.h`
