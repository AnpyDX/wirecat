# WireCat

A lightweight network protocol analyzer.

## Features

Supported protocols:

- Ethernet II
- IEEE 802.3
- ARP / RARP
- IPv4 / IPv6
- ICMP / ICMPv6
- UDP / TCP

## Requirements

- Windows / Linux / macOS
- CMake 4.0+
- C++ 20 Compiler
- OpenGL 3.3 support

## Build

### Preparations

First, clone repo into your computer:

```bash
git clone --recursive https://github.com/anpydx/wirecat.git
```

If you are compiling on Windows, you need to download and extract `WinPcap` or `NPcap` SDK in `~/sdk` directory. Check [sdk/README.md](https://github.com/AnpyDX/wirecat/tree/main/sdk).

If you are using Linux or macOS, check [PcapPlusPlus Documentation](https://pcapplusplus.github.io/docs/install) for dependencies installation and environment configuration.

### Compilation

Once setting up your environment, run following commands to start compiling for executable.

```bash
cmake -S . -B build
cmake --build build
```

Executable can be found in `~/bin`.

## Dependencies

**wirecat** use these libraries as dependencies:

- [GLAD](https://github.com/Dav1dde/glad)
- [GLFW](https://github.com/glfw/glfw)
- [ImGui](https://github.com/ocornut/imgui)
- [ImGui-Hex-Editor](https://github.com/Teselka/imgui_hex_editor)
- [PcapPlusPlus](https://github.com/seladb/PcapPlusPlus)

Additionally, **wirecat** use `YaHei-Consolas-Hybrid` as default UI font, TTF from:

- [YaHei-Consolas-Hybrid](https://github.com/yakumioto/YaHei-Consolas-Hybrid-1.12)

## License

Licensed under the MIT license, check [LICENSE](LICENSE) for details.
