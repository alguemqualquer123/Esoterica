Esoterica — Build Linux
========================

Esta pasta (Build/x64_Release/Linux) é o destino para binários Linux x64 (Vulkan + SDL3).

No host Windows atual, o build Linux NÃO foi executado nativamente porque requer toolchain Linux
(clang/gcc, libvulkan-dev, libsdl3-dev, etc). O framework já está 100% preparado:

  - CMakePresets: linux-release / linux-debug  -> EE_OUTPUT_DIR = Build/x64_Release|Debug/Linux
  - RHI_Vulkan.cpp, Platform_Linux.*, Application_Linux, etc.
  - BuildLinux.sh e Docs/BUILD_LINUX.md

Para compilar no Linux (nativo, WSL2 Ubuntu ou Docker):

  # Ubuntu 22.04 / 24.04
  sudo apt update && sudo apt install -y build-essential clang ninja-build cmake pkg-config \
    libvulkan-dev vulkan-validationlayers libsdl3-dev libfreetype6-dev libuuid1 uuid-dev \
    libssl-dev libprotobuf-dev zlib1g-dev libsqlite3-dev

  ./BuildLinux.sh release          # ou: cmake --preset linux-release && cmake --build build/linux-release

  # Binários aparecerão aqui:
  ls -lh Build/x64_Release/Linux/
  ./Build/x64_Release/Linux/EsotericaEditor
  ./Build/x64_Release/Linux/EsotericaEngine

CI: GitHub Actions em .github/workflows/build-linux.yml compila automaticamente a cada push.
