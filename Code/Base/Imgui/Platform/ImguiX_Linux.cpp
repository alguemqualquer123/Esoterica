#if EE_PLATFORM_LINUX
#include "Base/Imgui/ImguiX.h"
#include "Base/Imgui/ImguiSystem.h"

// Linux/SDL stub for ImguiX — mirrors ImguiX_Win32.cpp
// Full implementation uses SDL + Vulkan backend (imgui_impl_sdl3 + imgui_impl_vulkan)

#if EE_DEVELOPMENT_TOOLS
// Placeholder: actual ImGui Vulkan/SDL initialization lives in ImguiSystem.cpp
// which already has platform abstractions. This file ensures the build links.
#endif
#endif
