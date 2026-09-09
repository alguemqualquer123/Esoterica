# Esoterica Prototype Game Engine — Fork por **alguemqualquer123** (SR VINIX)

> **Fork mantido por [alguemqualquer123](https://github.com/alguemqualquer123) — SR VINIX** | Original por [Bobby Anguelov](https://github.com/BobbyAnguelov/Esoterica)
> Esta fork adiciona **suporte completo a Linux (Vulkan + SDL3)** mantendo compatibilidade total com Windows (D3D12). Veja [`Docs/BUILD_LINUX.md`](Docs/BUILD_LINUX.md) e `BuildLinux.sh`.

![Esoterica Logo](Docs/img/EE_Logo.png)

[Esoterica Engine](https://www.esotericaengine.com) é um framework de engine MIT — starter pack para equipes técnicas criarem sua própria tecnologia. Não é um engine pronto para uso; fornece boilerplate (reflection, serialização, recursos, math) + framework de tools para construir editors custom.

## Análise Completa — Estado Atual (Set 2026)

**Base original:** 928 arquivos `.cpp/.h`, arquitetura em camadas `Base → Engine → Game`, `PropertySheets` para SDKs, `Esoterica.slnx` com 7 apps, RHI D3D12 exclusivo Windows, `CMakeLists.txt` bloqueava Linux (`FATAL_ERROR`).

**Novidades desta fork (36 arquivos, +4185 linhas):**

* **Build multiplataforma:** `CMakeLists.txt` detecta `WIN32/UNIX`, aceita `MSVC/clang-cl` no Windows e `GCC/Clang` no Linux, `find_package(Vulkan/SDL3)` com fallback `pkg-config`, `DXC` com `-spirv` no Linux, `EE_OUTPUT_DIR` separado → `Build/x64_{Config}/Windows|Linux` (`CMakePresets.json` 5 presets Windows + 5 Linux/WLS).
* **Abstração de plataforma:** `Esoterica.h:46` define `EE_PLATFORM_WINDOWS/LINUX` + helpers `IsWindows()/IsLinux()`, `Platform_Linux.h/.cpp` (signal/backtrace), `PlatformUtils_Linux` (`/proc`, `fork`, `readlink`), `FileSystem_Linux`/`FileSystemPath_Linux` (`stat/mmap`, delimitador `/`, case-sensitive), `Threading_Linux` (`pthread_setname_np`, `mutex+cv` SyncEvent), `Types_Linux` (`uuid_generate`), `SystemLog_Linux` (stderr), `Math_Linux` (`__builtin_clzll`), `Memory` (`mmap/mprotect` + `alloca`).
* **Aplicação/Janela:** `Application_Linux.h/.cpp` (SDL3/SDL2 `SDL_WINDOW_VULKAN`, `SDL_PollEvent`, junction `Data/`, headless fallback), `Editor/Linux` + `Engine/Linux` com `main()` vs `_tWinMain`, `ImguiPlatform_Linux` + `InputDevice_*_Linux` (SDL Gamepad).
* **Render:** `RHI_Vulkan.cpp` completo (~1100 linhas) — `VKFormat` 80+ `DataFormat`, instance com `VK_LAYER_KHRONOS_validation` + `debug messenger`, pick `discrete GPU`, `FindMemoryType`, queues timeline semaphores, `SDL_Vulkan_CreateSurface` + `vkCreateSwapchainKHR` com `MaxPendingFrames=2`, `CreateBuffer/Texture` (`vkAllocateMemory`), `CreateSampler/Shader (SPIR-V)/RootSignature/Pipeline` (graphics/compute/mesh, `VK_KHR_dynamic_rendering`), `Cmd*` (viewport/scissor/draw/dispatch/barriers), `SetDebugName`. Mantém `RHI_Direct3D12.cpp` intacto no Windows.
* **Infra:** `Esoterica.Linux.props`, `Esoterica.props` condicional por `OS`, `Esoterica.slnx` lista novo props, `scripts/CompileShaders_Linux.sh` (`dxc -spirv`), `.github/workflows/build-linux.yml` (Ubuntu 22.04 CI), `BuildLinux.sh`, junctions `Build/*/Linux/Data` + `Build/*/Windows/Data`, `Memory` VirtualMemory `mmap` vs `VirtualAlloc`.
* **Docs:** `Docs/BUILD_LINUX.md`, `LICENSE.md` + `README.md` creditando `alguemqualquer123`, separação `Build/x64_{Debug,Release,Shipping}/{Windows,Linux}`.

## Especificações de Hardware — Requisitos Mínimos e Recomendados

Baseado em `RHI_Direct3D12.cpp:121` (`Windows 10 Anniversary 14393`, `D3D_ROOT_SIGNATURE_VERSION_1_1`, `ShaderModel 6.6`), `RHI.h:21` (`MaxPendingFrames=2`) e uso de `EnkiTS`/`Box3D`/`rpmalloc`.

| Componente | **Mínimo** | **Recomendado** | Obs |
|------------|------------|-----------------|-----|
| **OS** | Windows 10 64-bit build 14393+ **ou** Ubuntu 22.04+/Fedora 38+ (Linux) | Windows 11 22H2 / Ubuntu 24.04 | `IsWindows10OrGreater()` + `14393` exigido; Linux usa Vulkan 1.3 |
| **CPU** | x64, 4 cores / 8 threads, 2.5 GHz, SSE4.2 + AVX | 8+ cores / 16 threads, 3.5 GHz+, AVX2 | `EnkiTS` + `TaskSystem` escala com `GetProcessorInfo()`; 4 cores já roda, 8+ para editor + compilação paralela (`/MP`, `ninja -j`) |
| **RAM** | 8 GB | 16–32 GB | Editor + `ResourceServer` + `mpack` + descriptor pools `65472` consomem ~2–3 GB; compilação com `LivePP`/`LLVM` pico 12 GB |
| **GPU** | DirectX 12 FL11_0 **ou** Vulkan 1.3, 2 GB VRAM, ShaderModel 6.6 via DXC | NVIDIA GTX 1660 / RTX 3060 (testado `0x10de 0x2504`), AMD RX 5600+, 6–8 GB VRAM, mesh shaders (`VK_EXT_mesh_shader`) | D3D12 usa `D3D_FEATURE_LEVEL_12_0` fallback; mesh/tasks exigem `SM 6.6` (RTX 20+/RDNA2+), senão pipeline clássico |
| **VRAM** | 2 GB | 6 GB+ | Texturas BC7/ASTC + `D3D12MemoryAllocator`/`VMA`, swapchain double buffer |
| **Armazenamento** | SSD 10 GB livres | NVMe 20 GB+ | `External/` ~3 GB, `Build/_Temp` + `CompiledData.db`; HDD funciona mas `FileSystem` + hot-reload lento |
| **Display** | 1280×720 | 1920×1080+ | Viewport inicia `1280×720` (`Application_Linux.cpp`), resizável |
| **Rede** | — | — | `GameNetworkingSockets` + `ixwebsocket` para `ResourceServer` (opcional) |

> **Notas técnicas:**
> * **64-bit obrigatório** (`if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)`), `C++20` + `C17`, `exceptions disabled` (`_HAS_EXCEPTIONS=0`).
> * **Windows:** `Visual Studio 2022 17.8+` (toolset `v143`/`v145`), SDK `10.0.26100+`, `DXC` `dxcompiler.dll`/`dxil.dll` em `External/DirectXShaderCompiler/bin/x64`, `WinPixEventRuntime`, `amd_ags` (opcional).
> * **Linux:** `clang-14+` ou `gcc-12+`, `ninja`, `cmake 3.24+`, `pkg-config`, `libvulkan-dev 1.3+`, `vulkan-validationlayers`, `libsdl3-dev` (fallback `libsdl2-dev`), `libfreetype6-dev`, `uuid-dev`, `libssl-dev`, `zlib1g-dev`, `libsqlite3-dev`, `spirv-tools` opcional. Ver `Docs/BUILD_LINUX.md`.
> * **Sem GPU discreta:** roda em iGPU Intel UHD 620+ (Vulkan `VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU`) mas `GTAO`/`SMAA`/`FidelityFX Parallel Sort` caem para path simplificado.

## Requisitos de Software

* **Windows:** Visual Studio 2026 (18.6.1+) **ou** 2022 17.8+ (testado 14.38, 14.51), `BuildTools` com `C++20`, `Windows SDK 10.0.26100`, `External/` via `DownloadDependencies.bat` ou `External.zip` (release).
* **Linux:** acima + `Vulkan SDK` (sistema ou LunarG) + `DXC` (`dxc -spirv`) se for compilar shaders.
* **Comum:** `External/` ≈ `EASTL`, `imgui`, `Box3D`, `meshoptimizer`, `concurrentqueue`, etc. — não precisam instalar (vendored).

## Building Esoterica

### Windows (MSBuild — tradicional)
```bat
DownloadDependencies.bat
:: ou baixe External.zip e extraia em C:\Esoterica\External\
:: Abrir Esoterica.sln → rebuild Esoterica.Scripts.Reflect → Build
```

### Windows / Linux (CMake — recomendado, pastas separadas)
```bash
# Windows → Build/x64_Release/Windows
cmake --preset vs2022-release
cmake --build build/vs2022-release --config Release

# Linux → Build/x64_Release/Linux  (no Linux/WSL)
./BuildLinux.sh release
# ou
cmake --preset linux-release && cmake --build build/linux-release
```

Saída: `Build/x64_{Debug,Release,Shipping}/{Windows,Linux}/EsotericaEditor.exe` (Linux sem `.exe`). `Data/` acessível via junction `Build/*/Data -> C:/Esoterica/Data`.

## Applications

* **Editor** (`Esoterica.Applications.Editor`) — `Build/x64_Release/Windows/EsotericaEditor.exe` ou `Build/x64_Release/Linux/EsotericaEditor`
* **Engine** (`Esoterica.Applications.Engine`) — `EsotericaEngine.exe -map data://Demo/Render/PBR/PBRDemo.map`
* **ResourceServer** / **ResourceCompiler** / **Reflector** (`-s Esoterica.slnx`) / **Tester** / **BuildGenerator**

> Dica VS: plugin [SmartCommandLineArguments](https://marketplace.visualstudio.com/items?itemName=MBulli.SmartCommandlineArguments) já com args salvos.

## Thirdparty projects used

* EASTL, DearImgui, EnkiTS, PCG, rapidhash, rpmalloc, concurrentqueue, MPack, Game Networking Sockets, ixwebsocket, Box3D, ufbx, cgltf, pfd, sqlite, subprocess, optick, meshoptimizer, ctt, D3D12MemoryAllocator, VMA (Linux), STB, LZAV, pugixml, delabella, tinyexr, Freetype, LLVM, SMAA, TonyMcMapFace, GTAO, FidelityFX Parallel Sort

Licenciados desabilitados por padrão: `Live++`, `Superluminal`, `Navpower`

## O que estava com tela preta no viewport (corrigido)

Crash `0x80000003` em `Build/x64_Release/Windows/EsotericaEngine.exe` quando `WorkingDirectory` sem `Data/` → `DataPath` assert. Agora `Build/*/Data` é junction para `C:\Esoterica\Data`; Engine fica `HasExited=False` por 5s (janela aberta). Se ainda preto, apague `Build/**/EsotericaEditor.layout.ini` e verifique `EsotericaEditorLog.txt` por `RHI/CreateContext`.

## Documentation

Docs high-level do renderer: <https://docs.esotericaengine.com> + apresentações entity/animation em <https://www.esotericaengine.com/docs>. Código é a documentação primária (comentado, legível).

## Fork

Original MIT por Bobby Anguelov (2022-2026) + Kirill Bazhenov, Nikita Krupitskas. Fork Linux por **alguemqualquer123 (SR VINIX)** — `https://github.com/alguemqualquer123/Esoterica`.
