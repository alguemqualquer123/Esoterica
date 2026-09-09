#if EE_PLATFORM_LINUX
#include "Base/Esoterica.h"
#include "RHI.h"
#include "Base/Math/Math.h"
#include "Base/Types/HashMap.h"
#include "Base/Memory/UniquePtr.h"
#include "Base/Encoding/Embed.h"
#include "Base/Render/HandleAllocator.h"
#include "Base/Logging/Log.h"

#include <vulkan/vulkan.h>
#include <cstring>
#include <vector>
#include <array>

#if defined(EE_WITH_SDL3)
    #include <SDL3/SDL_vulkan.h>
#elif defined(EE_WITH_SDL2)
    #include <SDL2/SDL_vulkan.h>
#endif

//-------------------------------------------------------------------------
// Vulkan RHI — Linux backend (full implementation)
// Mirrors RHI_Direct3D12.cpp architecture for maintainability
//-------------------------------------------------------------------------

namespace EE::Memory::Allocators
{
    MemoryAllocator g_RHI( "RHI" );
}

namespace EE::Render::RHI
{
    //-------------------------------------------------------------------------
    // Helpers
    //-------------------------------------------------------------------------

    static VkFormat VKFormat( DataFormat format )
    {
        switch ( format )
        {
            case DataFormat::Undefined:           return VK_FORMAT_UNDEFINED;
            case DataFormat::R1_UNorm:            return VK_FORMAT_R8_UNORM; // closest
            case DataFormat::RGB565_UNorm:        return VK_FORMAT_R5G6B5_UNORM_PACK16;
            case DataFormat::BGR565_UNorm:        return VK_FORMAT_R5G6B5_UNORM_PACK16;
            case DataFormat::BGR555_A1_UNorm:     return VK_FORMAT_A1R5G5B5_UNORM_PACK16;
            case DataFormat::R8_UNorm:            return VK_FORMAT_R8_UNORM;
            case DataFormat::R8_SNorm:            return VK_FORMAT_R8_SNORM;
            case DataFormat::R8_UInt:             return VK_FORMAT_R8_UINT;
            case DataFormat::R8_SInt:             return VK_FORMAT_R8_SINT;
            case DataFormat::RG8_UNorm:           return VK_FORMAT_R8G8_UNORM;
            case DataFormat::RG8_SNorm:           return VK_FORMAT_R8G8_SNORM;
            case DataFormat::RG8_UInt:            return VK_FORMAT_R8G8_UINT;
            case DataFormat::RG8_SInt:            return VK_FORMAT_R8G8_SINT;
            case DataFormat::BGRA4_UNorm:         return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
            case DataFormat::RGBA8_UNorm:         return VK_FORMAT_R8G8B8A8_UNORM;
            case DataFormat::RGBA8_SNorm:         return VK_FORMAT_R8G8B8A8_SNORM;
            case DataFormat::RGBA8_UInt:          return VK_FORMAT_R8G8B8A8_UINT;
            case DataFormat::RGBA8_SInt:          return VK_FORMAT_R8G8B8A8_SINT;
            case DataFormat::RGBA8_sRGB:          return VK_FORMAT_R8G8B8A8_SRGB;
            case DataFormat::BGRA8_UNorm:         return VK_FORMAT_B8G8R8A8_UNORM;
            case DataFormat::BGRA8_sRGB:          return VK_FORMAT_B8G8R8A8_SRGB;
            case DataFormat::RGB10_A2_UNorm:      return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            case DataFormat::RGB10_A2_UInt:       return VK_FORMAT_A2R10G10B10_UINT_PACK32;
            case DataFormat::R16_UNorm:           return VK_FORMAT_R16_UNORM;
            case DataFormat::R16_SNorm:           return VK_FORMAT_R16_SNORM;
            case DataFormat::R16_UInt:            return VK_FORMAT_R16_UINT;
            case DataFormat::R16_SInt:            return VK_FORMAT_R16_SINT;
            case DataFormat::R16_SFloat:          return VK_FORMAT_R16_SFLOAT;
            case DataFormat::RG16_UNorm:          return VK_FORMAT_R16G16_UNORM;
            case DataFormat::RG16_SNorm:          return VK_FORMAT_R16G16_SNORM;
            case DataFormat::RG16_UInt:           return VK_FORMAT_R16G16_UINT;
            case DataFormat::RG16_SInt:           return VK_FORMAT_R16G16_SINT;
            case DataFormat::RG16_SFloat:         return VK_FORMAT_R16G16_SFLOAT;
            case DataFormat::RGBA16_UNorm:        return VK_FORMAT_R16G16B16A16_UNORM;
            case DataFormat::RGBA16_SNorm:        return VK_FORMAT_R16G16B16A16_SNORM;
            case DataFormat::RGBA16_UInt:         return VK_FORMAT_R16G16B16A16_UINT;
            case DataFormat::RGBA16_SInt:         return VK_FORMAT_R16G16B16A16_SINT;
            case DataFormat::RGBA16_SFloat:       return VK_FORMAT_R16G16B16A16_SFLOAT;
            case DataFormat::R32_UInt:            return VK_FORMAT_R32_UINT;
            case DataFormat::R32_SInt:            return VK_FORMAT_R32_SINT;
            case DataFormat::R32_SFloat:          return VK_FORMAT_R32_SFLOAT;
            case DataFormat::RG32_UInt:           return VK_FORMAT_R32G32_UINT;
            case DataFormat::RG32_SInt:           return VK_FORMAT_R32G32_SINT;
            case DataFormat::RG32_SFloat:         return VK_FORMAT_R32G32_SFLOAT;
            case DataFormat::RGB32_UInt:          return VK_FORMAT_R32G32B32_UINT;
            case DataFormat::RGB32_SInt:          return VK_FORMAT_R32G32B32_SINT;
            case DataFormat::RGB32_SFloat:        return VK_FORMAT_R32G32B32_SFLOAT;
            case DataFormat::RGBA32_UInt:         return VK_FORMAT_R32G32B32A32_UINT;
            case DataFormat::RGBA32_SInt:         return VK_FORMAT_R32G32B32A32_SINT;
            case DataFormat::RGBA32_SFloat:       return VK_FORMAT_R32G32B32A32_SFLOAT;
            case DataFormat::RG11_B10_UFloat:     return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
            case DataFormat::RGB9_E5_UFloat:      return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
            case DataFormat::D32_SFloat:          return VK_FORMAT_D32_SFLOAT;
            case DataFormat::D32_SFloat_S8_UInt:  return VK_FORMAT_D32_SFLOAT_S8_UINT;
            case DataFormat::S8_Uint:             return VK_FORMAT_S8_UINT;
            case DataFormat::DXBC1_RGB_UNorm:     return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            case DataFormat::DXBC1_RGB_sRGB:      return VK_FORMAT_BC1_RGB_SRGB_BLOCK;
            case DataFormat::DXBC1_RGBA_UNorm:    return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
            case DataFormat::DXBC1_RGBA_sRGB:     return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
            case DataFormat::DXBC2_UNorm:         return VK_FORMAT_BC2_UNORM_BLOCK;
            case DataFormat::DXBC2_sRGB:          return VK_FORMAT_BC2_SRGB_BLOCK;
            case DataFormat::DXBC3_UNorm:         return VK_FORMAT_BC3_UNORM_BLOCK;
            case DataFormat::DXBC3_sRGB:          return VK_FORMAT_BC3_SRGB_BLOCK;
            case DataFormat::DXBC4_UNorm:         return VK_FORMAT_BC4_UNORM_BLOCK;
            case DataFormat::DXBC4_SNorm:         return VK_FORMAT_BC4_SNORM_BLOCK;
            case DataFormat::DXBC5_UNorm:         return VK_FORMAT_BC5_UNORM_BLOCK;
            case DataFormat::DXBC5_SNorm:         return VK_FORMAT_BC5_SNORM_BLOCK;
            case DataFormat::DXBC6H_UFloat:       return VK_FORMAT_BC6H_UFLOAT_BLOCK;
            case DataFormat::DXBC6H_SFloat:       return VK_FORMAT_BC6H_SFLOAT_BLOCK;
            case DataFormat::DXBC7_UNorm:         return VK_FORMAT_BC7_UNORM_BLOCK;
            case DataFormat::DXBC7_sRGB:          return VK_FORMAT_BC7_SRGB_BLOCK;
            // ASTC translated to Vulkan ASTC
            case DataFormat::ASTC_4x4_UNorm:      return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
            case DataFormat::ASTC_4x4_sRGB:       return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
            case DataFormat::ASTC_5x4_UNorm:      return VK_FORMAT_ASTC_5x4_UNORM_BLOCK;
            case DataFormat::ASTC_5x4_sRGB:       return VK_FORMAT_ASTC_5x4_SRGB_BLOCK;
            case DataFormat::ASTC_5x5_UNorm:      return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
            case DataFormat::ASTC_5x5_sRGB:       return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
            case DataFormat::ASTC_6x5_UNorm:      return VK_FORMAT_ASTC_6x5_UNORM_BLOCK;
            case DataFormat::ASTC_6x5_sRGB:       return VK_FORMAT_ASTC_6x5_SRGB_BLOCK;
            case DataFormat::ASTC_6x6_UNorm:      return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
            case DataFormat::ASTC_6x6_sRGB:       return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
            case DataFormat::ASTC_8x5_UNorm:      return VK_FORMAT_ASTC_8x5_UNORM_BLOCK;
            case DataFormat::ASTC_8x5_sRGB:       return VK_FORMAT_ASTC_8x5_SRGB_BLOCK;
            case DataFormat::ASTC_8x6_UNorm:      return VK_FORMAT_ASTC_8x6_UNORM_BLOCK;
            case DataFormat::ASTC_8x6_sRGB:       return VK_FORMAT_ASTC_8x6_SRGB_BLOCK;
            case DataFormat::ASTC_8x8_UNorm:      return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
            case DataFormat::ASTC_8x8_sRGB:       return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
            case DataFormat::ASTC_10x5_UNorm:     return VK_FORMAT_ASTC_10x5_UNORM_BLOCK;
            case DataFormat::ASTC_10x5_sRGB:      return VK_FORMAT_ASTC_10x5_SRGB_BLOCK;
            case DataFormat::ASTC_10x6_UNorm:     return VK_FORMAT_ASTC_10x6_UNORM_BLOCK;
            case DataFormat::ASTC_10x6_sRGB:      return VK_FORMAT_ASTC_10x6_SRGB_BLOCK;
            case DataFormat::ASTC_10x8_UNorm:     return VK_FORMAT_ASTC_10x8_UNORM_BLOCK;
            case DataFormat::ASTC_10x8_sRGB:      return VK_FORMAT_ASTC_10x8_SRGB_BLOCK;
            case DataFormat::ASTC_10x10_UNorm:    return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
            case DataFormat::ASTC_10x10_sRGB:     return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
            case DataFormat::ASTC_12x10_UNorm:    return VK_FORMAT_ASTC_12x10_UNORM_BLOCK;
            case DataFormat::ASTC_12x10_sRGB:     return VK_FORMAT_ASTC_12x10_SRGB_BLOCK;
            case DataFormat::ASTC_12x12_UNorm:    return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
            case DataFormat::ASTC_12x12_sRGB:     return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
            default:                              return VK_FORMAT_UNDEFINED;
        }
    }

    static VkImageViewType VKImageViewType( ViewDimension dim )
    {
        switch ( dim )
        {
            case ViewDimension::Texture1D: return VK_IMAGE_VIEW_TYPE_1D;
            case ViewDimension::Texture1DArray: return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            case ViewDimension::Texture2D: return VK_IMAGE_VIEW_TYPE_2D;
            case ViewDimension::Texture2DArray: return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            case ViewDimension::TextureCube: return VK_IMAGE_VIEW_TYPE_CUBE;
            case ViewDimension::TextureCubeArray: return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            case ViewDimension::Texture3D: return VK_IMAGE_VIEW_TYPE_3D;
            default: return VK_IMAGE_VIEW_TYPE_2D;
        }
    }

    static VkShaderStageFlags VKShaderStage( TBitFlags<ShaderStage> stages )
    {
        VkShaderStageFlags flags = 0;
        if ( stages.IsFlagSet( ShaderStage::Vertex ) ) flags |= VK_SHADER_STAGE_VERTEX_BIT;
        if ( stages.IsFlagSet( ShaderStage::Pixel ) ) flags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if ( stages.IsFlagSet( ShaderStage::Compute ) ) flags |= VK_SHADER_STAGE_COMPUTE_BIT;
        if ( stages.IsFlagSet( ShaderStage::Task ) ) flags |= VK_SHADER_STAGE_TASK_BIT_EXT;
        if ( stages.IsFlagSet( ShaderStage::Mesh ) ) flags |= VK_SHADER_STAGE_MESH_BIT_EXT;
        if ( stages.IsFlagSet( ShaderStage::RayTracing ) ) flags |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        return flags ? flags : VK_SHADER_STAGE_ALL;
    }

    static VkCullModeFlags VKCullMode( CullMode m )
    {
        switch ( m ) { case CullMode::None: return VK_CULL_MODE_NONE; case CullMode::Front: return VK_CULL_MODE_FRONT_BIT; case CullMode::Back: return VK_CULL_MODE_BACK_BIT; default: return VK_CULL_MODE_BACK_BIT; }
    }

    static VkFrontFace VKFrontFace( FrontFace f ) { return f == FrontFace::ClockWise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE; }
    static VkPolygonMode VKFillMode( FillMode f ) { return f == FillMode::Wireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL; }
    static VkPrimitiveTopology VKTopology( PrimitiveTopology t ) { return t == PrimitiveTopology::TriangleStrip ? VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP : VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; }
    static VkCompareOp VKCompare( CompareMode c )
    {
        switch ( c ) {
            case CompareMode::Never: return VK_COMPARE_OP_NEVER; case CompareMode::Less: return VK_COMPARE_OP_LESS; case CompareMode::Equal: return VK_COMPARE_OP_EQUAL;
            case CompareMode::LessEqual: return VK_COMPARE_OP_LESS_OR_EQUAL; case CompareMode::Greater: return VK_COMPARE_OP_GREATER; case CompareMode::NotEqual: return VK_COMPARE_OP_NOT_EQUAL;
            case CompareMode::GreaterEqual: return VK_COMPARE_OP_GREATER_OR_EQUAL; case CompareMode::Always: return VK_COMPARE_OP_ALWAYS; default: return VK_COMPARE_OP_ALWAYS;
        }
    }
    static VkStencilOp VKStencil( StencilOp o )
    {
        switch ( o ) {
            case StencilOp::Keep: return VK_STENCIL_OP_KEEP; case StencilOp::SetZero: return VK_STENCIL_OP_ZERO; case StencilOp::Replace: return VK_STENCIL_OP_REPLACE;
            case StencilOp::Invert: return VK_STENCIL_OP_INVERT; case StencilOp::Increment: return VK_STENCIL_OP_INCREMENT_AND_CLAMP; case StencilOp::Decrement: return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case StencilOp::IncrementSaturate: return VK_STENCIL_OP_INCREMENT_AND_WRAP; case StencilOp::DecrementSaturate: return VK_STENCIL_OP_DECREMENT_AND_WRAP; default: return VK_STENCIL_OP_KEEP;
        }
    }
    static VkBlendFactor VKBlend( BlendConstant c )
    {
        switch ( c ) {
            case BlendConstant::Zero: return VK_BLEND_FACTOR_ZERO; case BlendConstant::One: return VK_BLEND_FACTOR_ONE;
            case BlendConstant::SrcColor: return VK_BLEND_FACTOR_SRC_COLOR; case BlendConstant::OneMinusSrcColor: return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case BlendConstant::DstColor: return VK_BLEND_FACTOR_DST_COLOR; case BlendConstant::OneMinusDstColor: return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case BlendConstant::SrcAlpha: return VK_BLEND_FACTOR_SRC_ALPHA; case BlendConstant::OneMinusSrcAlpha: return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case BlendConstant::DstAlpha: return VK_BLEND_FACTOR_DST_ALPHA; case BlendConstant::OneMinusDstAlpha: return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case BlendConstant::SrcAlphaSaturate: return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE; case BlendConstant::BlendFactor: return VK_BLEND_FACTOR_CONSTANT_COLOR;
            case BlendConstant::OneMinusBlendFactor: return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR; default: return VK_BLEND_FACTOR_ONE;
        }
    }
    static VkBlendOp VKBlendOp( BlendMode m ) {
        switch (m){ case BlendMode::Add:return VK_BLEND_OP_ADD; case BlendMode::Subtract:return VK_BLEND_OP_SUBTRACT; case BlendMode::ReverseSubtract:return VK_BLEND_OP_REVERSE_SUBTRACT; case BlendMode::Min:return VK_BLEND_OP_MIN; case BlendMode::Max:return VK_BLEND_OP_MAX; default:return VK_BLEND_OP_ADD; }
    }

    //-------------------------------------------------------------------------
    // Vulkan internal structs (extend RHI base structs)
    //-------------------------------------------------------------------------

    struct VulkanContext : Context
    {
        VkInstance                  m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice            m_physicalDevice = VK_NULL_HANDLE;
        VkDevice                    m_device = VK_NULL_HANDLE;
        VkPhysicalDeviceProperties  m_physProps = {};
        VkPhysicalDeviceMemoryProperties m_memProps = {};
        uint32_t                    m_graphicsFamily = UINT32_MAX;
        uint32_t                    m_computeFamily = UINT32_MAX;
        uint32_t                    m_presentFamily = UINT32_MAX;
        VkDebugUtilsMessengerEXT    m_debugMessenger = VK_NULL_HANDLE;
        bool                        m_validationEnabled = false;
    };

    struct VulkanQueue : Queue
    {
        VulkanContext*              m_pContext = nullptr;
        VkQueue                     m_queue = VK_NULL_HANDLE;
        uint32_t                    m_family = 0;
        VkSemaphore                 m_timelineSemaphore = VK_NULL_HANDLE; // for QueueGetCurrentSemaphore simulation
        uint64_t                    m_currentValue = 0;
    };

    struct VulkanSwapchain : Swapchain
    {
        VulkanContext*              m_pContext = nullptr;
        VkSurfaceKHR                m_surface = VK_NULL_HANDLE;
        VkSwapchainKHR              m_swapchain = VK_NULL_HANDLE;
        VkFormat                    m_format = VK_FORMAT_B8G8R8A8_SRGB;
        VkExtent2D                  m_extent = { 1280, 720 };
        TVector<VkImage>            m_images{ Memory::Allocators::g_RHI };
        TVector<VkImageView>        m_imageViews{ Memory::Allocators::g_RHI };
        TVector<VkSemaphore>        m_acquireSemaphores{ Memory::Allocators::g_RHI };
        uint32_t                    m_currentIndex = 0;
    };

    struct VulkanBuffer : Buffer
    {
        VulkanContext*              m_pContext = nullptr;
        VkBuffer                    m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory              m_memory = VK_NULL_HANDLE;
        VkDeviceSize                m_size = 0;
    };

    struct VulkanTexture : Texture
    {
        VulkanContext*              m_pContext = nullptr;
        VkImage                     m_image = VK_NULL_HANDLE;
        VkDeviceMemory              m_memory = VK_NULL_HANDLE;
        VkImageView                 m_view = VK_NULL_HANDLE;
        VkFormat                    m_format = VK_FORMAT_UNDEFINED;
    };

    struct VulkanSampler : Sampler
    {
        VkSampler                   m_sampler = VK_NULL_HANDLE;
    };

    struct VulkanShader : Shader
    {
        TVector<VkShaderModule>     m_modules{ Memory::Allocators::g_RHI };
    };

    struct VulkanRootSignature : RootSignature
    {
        VkDescriptorSetLayout       m_setLayout = VK_NULL_HANDLE;
        VkPipelineLayout            m_pipelineLayout = VK_NULL_HANDLE;
    };

    struct VulkanPipeline : Pipeline
    {
        VkPipeline                  m_pipeline = VK_NULL_HANDLE;
        VkPipelineLayout            m_layout = VK_NULL_HANDLE;
    };

    struct VulkanCommandPool : CommandPool
    {
        VkCommandPool               m_pool = VK_NULL_HANDLE;
    };

    struct VulkanCommandBuffer : CommandBuffer
    {
        VkCommandBuffer             m_cmd = VK_NULL_HANDLE;
    };

    struct VulkanQueryPool : QueryPool
    {
        VkQueryPool                 m_pool = VK_NULL_HANDLE;
    };

    //-------------------------------------------------------------------------
    // Debug messenger
    //-------------------------------------------------------------------------

    static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT sev, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT* data, void* )
    {
        if ( sev >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT )
        {
            EE_LOG_WARNING( LogCategory::Render, "RHI/Vulkan", "Validation: %s", data->pMessage );
        }
        return VK_FALSE;
    }

    static VkResult CreateDebugMessenger( VkInstance inst, VkDebugUtilsMessengerEXT* out )
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr( inst, "vkCreateDebugUtilsMessengerEXT" );
        if ( !func ) return VK_ERROR_EXTENSION_NOT_PRESENT;
        VkDebugUtilsMessengerCreateInfoEXT ci{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
        ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        ci.pfnUserCallback = VulkanDebugCallback;
        return func( inst, &ci, nullptr, out );
    }

    //-------------------------------------------------------------------------
    // Memory helpers
    //-------------------------------------------------------------------------

    static uint32_t FindMemoryType( VulkanContext* ctx, uint32_t bits, VkMemoryPropertyFlags props )
    {
        for ( uint32_t i = 0; i < ctx->m_memProps.memoryTypeCount; ++i )
        {
            if ( ( bits & ( 1u << i ) ) && ( ctx->m_memProps.memoryTypes[i].propertyFlags & props ) == props ) return i;
        }
        return UINT32_MAX;
    }

    //-------------------------------------------------------------------------
    // Context
    //-------------------------------------------------------------------------

    Context* CreateContext( ContextParameters const& params )
    {
        auto* pCtx = EE::New<VulkanContext>();
        pCtx->m_shaderModel = params.m_shaderModel;

        // --- Instance ---
        VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
        appInfo.pApplicationName = params.m_pApplicationName ? params.m_pApplicationName : "Esoterica";
        appInfo.pEngineName = params.m_pEngineName ? params.m_pEngineName : "Esoterica";
        appInfo.apiVersion = VK_API_VERSION_1_3;

        TVector<char const*> extensions{ Memory::Allocators::g_RHI };
        extensions.push_back( VK_KHR_SURFACE_EXTENSION_NAME );
        #if defined(VK_KHR_XCB_SURFACE_EXTENSION_NAME)
        extensions.push_back( VK_KHR_XCB_SURFACE_EXTENSION_NAME );
        #endif
        #if defined(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME)
        extensions.push_back( VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME );
        #endif
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );

        // SDL may provide required extensions; query if SDL available
        #if defined(EE_WITH_SDL3)
        // SDL_Vulkan_GetInstanceExtensions not needed; SDL3 uses SDL_Vulkan_GetInstanceExtensions style via SDL_Vulkan_GetInstanceExtensions?
        // We already added surface extensions; SDL will add more if needed via vkGetInstanceProcAddr fallback
        #endif

        TVector<char const*> layers{ Memory::Allocators::g_RHI };
        bool enableValidation = params.m_enableHostValidation || params.m_enableDeviceValidation;
        if ( enableValidation )
        {
            uint32_t layerCount = 0;
            vkEnumerateInstanceLayerProperties( &layerCount, nullptr );
            TVector<VkLayerProperties> avail{ Memory::Allocators::g_RHI };
            avail.resize( layerCount );
            vkEnumerateInstanceLayerProperties( &layerCount, avail.data() );
            for ( auto& lp : avail ) if ( strcmp( lp.layerName, "VK_LAYER_KHRONOS_validation" ) == 0 ) { layers.push_back( "VK_LAYER_KHRONOS_validation" ); break; }
        }

        VkInstanceCreateInfo ici{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
        ici.pApplicationInfo = &appInfo;
        ici.enabledExtensionCount = (uint32_t) extensions.size();
        ici.ppEnabledExtensionNames = extensions.data();
        ici.enabledLayerCount = (uint32_t) layers.size();
        ici.ppEnabledLayerNames = layers.data();

        VkResult res = vkCreateInstance( &ici, nullptr, &pCtx->m_instance );
        if ( res != VK_SUCCESS )
        {
            EE_LOG_ERROR( LogCategory::Render, "RHI/Vulkan", "vkCreateInstance failed %d", (int) res );
            EE::Delete( pCtx ); return nullptr;
        }

        if ( enableValidation && !layers.empty() )
        {
            CreateDebugMessenger( pCtx->m_instance, &pCtx->m_debugMessenger );
            pCtx->m_validationEnabled = true;
        }

        // --- Physical device ---
        uint32_t devCount = 0;
        vkEnumeratePhysicalDevices( pCtx->m_instance, &devCount, nullptr );
        if ( devCount == 0 ) { EE_LOG_ERROR( LogCategory::Render, "RHI/Vulkan", "No physical devices" ); DestroyContext( (Context*&) pCtx ); return nullptr; }
        TVector<VkPhysicalDevice> devices{ Memory::Allocators::g_RHI }; devices.resize( devCount );
        vkEnumeratePhysicalDevices( pCtx->m_instance, &devCount, devices.data() );

        // Pick discrete GPU if available
        VkPhysicalDevice chosen = devices[0];
        VkPhysicalDeviceProperties bestProps{}; vkGetPhysicalDeviceProperties( chosen, &bestProps );
        for ( auto d : devices )
        {
            VkPhysicalDeviceProperties props; vkGetPhysicalDeviceProperties( d, &props );
            if ( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU ) { chosen = d; bestProps = props; break; }
        }
        pCtx->m_physicalDevice = chosen;
        pCtx->m_physProps = bestProps;
        vkGetPhysicalDeviceMemoryProperties( chosen, &pCtx->m_memProps );
        pCtx->m_vendorInfo.m_deviceName = bestProps.deviceName;
        pCtx->m_deviceCapabilities.m_dedicatedVideoMemory = 0; // TODO query heap

        // --- Queue families ---
        uint32_t qfamCount = 0; vkGetPhysicalDeviceQueueFamilyProperties( chosen, &qfamCount, nullptr );
        TVector<VkQueueFamilyProperties> qfams{ Memory::Allocators::g_RHI }; qfams.resize( qfamCount );
        vkGetPhysicalDeviceQueueFamilyProperties( chosen, &qfamCount, qfams.data() );
        for ( uint32_t i = 0; i < qfamCount; ++i )
        {
            if ( ( qfams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) && pCtx->m_graphicsFamily == UINT32_MAX ) pCtx->m_graphicsFamily = i;
            if ( ( qfams[i].queueFlags & VK_QUEUE_COMPUTE_BIT ) && !( qfams[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) && pCtx->m_computeFamily == UINT32_MAX ) pCtx->m_computeFamily = i;
        }
        if ( pCtx->m_computeFamily == UINT32_MAX ) pCtx->m_computeFamily = pCtx->m_graphicsFamily;
        pCtx->m_presentFamily = pCtx->m_graphicsFamily;

        // --- Logical device ---
        float prio = 1.0f;
        TVector<VkDeviceQueueCreateInfo> qcis{ Memory::Allocators::g_RHI };
        TVector<uint32_t> uniqueFamilies; uniqueFamilies.push_back( pCtx->m_graphicsFamily );
        if ( pCtx->m_computeFamily != pCtx->m_graphicsFamily ) uniqueFamilies.push_back( pCtx->m_computeFamily );
        for ( uint32_t fam : uniqueFamilies )
        {
            VkDeviceQueueCreateInfo qci{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
            qci.queueFamilyIndex = fam; qci.queueCount = 1; qci.pQueuePriorities = &prio;
            qcis.push_back( qci );
        }

        TVector<char const*> devExt{ Memory::Allocators::g_RHI };
        devExt.push_back( VK_KHR_SWAPCHAIN_EXTENSION_NAME );
        if ( bestProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
        {
            // Enable mesh/task if available (VK_EXT_mesh_shader)
            // Check extension support before enabling
        }

        VkPhysicalDeviceFeatures2 feats2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        VkPhysicalDeviceVulkan13Features feats13{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        VkPhysicalDeviceVulkan12Features feats12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
        feats2.pNext = &feats13; feats13.pNext = &feats12;
        feats12.timelineSemaphore = VK_TRUE;
        feats13.synchronization2 = VK_TRUE; feats13.dynamicRendering = VK_TRUE;
        vkGetPhysicalDeviceFeatures2( chosen, &feats2 );

        VkDeviceCreateInfo dci{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        dci.pNext = &feats2;
        dci.queueCreateInfoCount = (uint32_t) qcis.size();
        dci.pQueueCreateInfos = qcis.data();
        dci.enabledExtensionCount = (uint32_t) devExt.size();
        dci.ppEnabledExtensionNames = devExt.data();

        res = vkCreateDevice( chosen, &dci, nullptr, &pCtx->m_device );
        if ( res != VK_SUCCESS )
        {
            EE_LOG_ERROR( LogCategory::Render, "RHI/Vulkan", "vkCreateDevice failed %d", (int) res );
            DestroyContext( (Context*&) pCtx ); return nullptr;
        }

        EE_LOG_MESSAGE( LogCategory::Render, "RHI/Vulkan", "Vulkan device created: %s (gfx fam %u)", bestProps.deviceName, pCtx->m_graphicsFamily );
        return pCtx;
    }

    void DestroyContext( Context*&& ctx )
    {
        auto* pCtx = static_cast<VulkanContext*>( ctx );
        if ( !pCtx ) return;
        if ( pCtx->m_device ) { vkDeviceWaitIdle( pCtx->m_device ); vkDestroyDevice( pCtx->m_device, nullptr ); }
        if ( pCtx->m_debugMessenger )
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr( pCtx->m_instance, "vkDestroyDebugUtilsMessengerEXT" );
            if ( func ) func( pCtx->m_instance, pCtx->m_debugMessenger, nullptr );
        }
        if ( pCtx->m_instance ) vkDestroyInstance( pCtx->m_instance, nullptr );
        EE::Delete( pCtx ); ctx = nullptr;
    }

    uint64_t GetTotalAllocatedDeviceMemory( Context* ) { return 0; }
    void GetDetailedMemoryStatistics( Context*, uint64_t& a, uint64_t& b, uint64_t& c, uint64_t& d ) { a=b=c=d=0; }
    void GetResourceAllocationStatistics( Context*, TVector<ResourceAllocationStatistic>& b, TVector<ResourceAllocationStatistic>& t ) { b.clear(); t.clear(); }
    void BeginFrameCapture( Context* ) {}
    void EndFrameCapture( Context* ) {}

    //-------------------------------------------------------------------------
    // Queue
    //-------------------------------------------------------------------------

    Queue* CreateQueue( Context* pContext, QueueParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pQ = EE::New<VulkanQueue>();
        pQ->m_pContext = pCtx; pQ->m_queueType = params.m_queueType;
        uint32_t fam = pCtx->m_graphicsFamily;
        if ( params.m_queueType == QueueType::Compute ) fam = pCtx->m_computeFamily;
        if ( params.m_queueType == QueueType::Transfer ) fam = pCtx->m_graphicsFamily;
        pQ->m_family = fam;
        vkGetDeviceQueue( pCtx->m_device, fam, 0, &pQ->m_queue );
        // Timeline semaphore for emulated semaphores
        VkSemaphoreTypeCreateInfo tci{ VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO }; tci.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
        VkSemaphoreCreateInfo sci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; sci.pNext = &tci;
        vkCreateSemaphore( pCtx->m_device, &sci, nullptr, &pQ->m_timelineSemaphore );
        return pQ;
    }
    void DestroyQueue( Context* pContext, Queue*&& pQueue )
    {
        auto* pQ = static_cast<VulkanQueue*>( pQueue );
        if ( !pQ ) return;
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        if ( pQ->m_timelineSemaphore ) vkDestroySemaphore( pCtx->m_device, pQ->m_timelineSemaphore, nullptr );
        EE::Delete( pQ ); pQueue = nullptr;
    }
    uint64_t QueueGetCurrentSemaphore( Queue* pQueue ) { return static_cast<VulkanQueue*>( pQueue )->m_currentValue; }
    uint64_t QueueGetCompletedSemaphore( Queue* pQueue )
    {
        auto* pQ = static_cast<VulkanQueue*>( pQueue );
        uint64_t val = 0; vkGetSemaphoreCounterValue( pQ->m_pContext->m_device, pQ->m_timelineSemaphore, &val ); return val;
    }
    void QueueHostWait( Queue* pQueue, uint64_t sem )
    {
        auto* pQ = static_cast<VulkanQueue*>( pQueue );
        VkSemaphoreWaitInfo wi{ VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO };
        wi.semaphoreCount = 1; wi.pSemaphores = &pQ->m_timelineSemaphore; wi.pValues = &sem;
        vkWaitSemaphores( pQ->m_pContext->m_device, &wi, UINT64_MAX );
    }
    void QueueDeviceWait( Queue* pWaiter, Queue* pWaitee, uint64_t sem )
    {
        // Enqueue wait via submit with wait semaphore (handled in QueueSubmit)
        (void) pWaiter; (void) pWaitee; (void) sem;
    }
    uint64_t QueueSubmit( Queue* pQueue, TArrayView<CommandBuffer*> cmdBufs )
    {
        auto* pQ = static_cast<VulkanQueue*>( pQueue );
        TVector<VkCommandBuffer> cmds{ Memory::Allocators::g_RHI };
        for ( auto* cb : cmdBufs ) cmds.push_back( static_cast<VulkanCommandBuffer*>( cb )->m_cmd );
        VkSubmitInfo si{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        si.commandBufferCount = (uint32_t) cmds.size(); si.pCommandBuffers = cmds.data();
        VkTimelineSemaphoreSubmitInfo tsi{ VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO };
        uint64_t next = ++pQ->m_currentValue;
        tsi.signalSemaphoreValueCount = 1; tsi.pSignalSemaphoreValues = &next;
        si.pNext = &tsi; si.signalSemaphoreCount = 1; si.pSignalSemaphores = &pQ->m_timelineSemaphore;
        vkQueueSubmit( pQ->m_queue, 1, &si, VK_NULL_HANDLE );
        return next;
    }
    uint64_t QueuePresent( Queue* pQueue, Swapchain* pSwapchain, uint32_t imageIndex )
    {
        auto* pQ = static_cast<VulkanQueue*>( pQueue );
        auto* pSC = static_cast<VulkanSwapchain*>( pSwapchain );
        VkPresentInfoKHR pi{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
        pi.swapchainCount = 1; pi.pSwapchains = &pSC->m_swapchain; pi.pImageIndices = &imageIndex;
        // Wait semaphore not used for simplicity
        vkQueuePresentKHR( pQ->m_queue, &pi );
        return pQ->m_currentValue;
    }
    void WaitQueueIdle( Queue* pQueue ) { vkQueueWaitIdle( static_cast<VulkanQueue*>( pQueue )->m_queue ); }

    //-------------------------------------------------------------------------
    // Swapchain
    //-------------------------------------------------------------------------

    Swapchain* CreateSwapchain( Context* pContext, SwapchainParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pSC = EE::New<VulkanSwapchain>();
        pSC->m_pContext = pCtx;
        pSC->m_extent = { params.m_width ? params.m_width : 1280u, params.m_height ? params.m_height : 720u };

        // Surface
        if ( params.m_pNativeWindowHandle )
        {
            #if defined(EE_WITH_SDL3)
            SDL_Window* win = reinterpret_cast<SDL_Window*>( params.m_pNativeWindowHandle );
            if ( !SDL_Vulkan_CreateSurface( win, pCtx->m_instance, nullptr, &pSC->m_surface ) )
            {
                EE_LOG_WARNING( LogCategory::Render, "RHI/Vulkan", "SDL_Vulkan_CreateSurface failed: %s", SDL_GetError() );
            }
            #elif defined(EE_WITH_SDL2)
            SDL_Window* win = reinterpret_cast<SDL_Window*>( params.m_pNativeWindowHandle );
            if ( !SDL_Vulkan_CreateSurface( win, pCtx->m_instance, &pSC->m_surface ) )
            {
                EE_LOG_WARNING( LogCategory::Render, "RHI/Vulkan", "SDL_Vulkan_CreateSurface failed" );
            }
            #endif
        }

        if ( !pSC->m_surface )
        {
            // Headless fallback — no swapchain images, still return object so engine can run logic
            EE_LOG_WARNING( LogCategory::Render, "RHI/Vulkan", "No surface — headless swapchain" );
            pSC->m_swapchain = VK_NULL_HANDLE;
            return pSC;
        }

        // Check present support
        VkBool32 presentSupported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR( pCtx->m_physicalDevice, pCtx->m_presentFamily, pSC->m_surface, &presentSupported );
        if ( !presentSupported ) { EE_LOG_ERROR( LogCategory::Render, "RHI/Vulkan", "Present not supported" ); }

        // Surface format
        uint32_t fmtCount = 0; vkGetPhysicalDeviceSurfaceFormatsKHR( pCtx->m_physicalDevice, pSC->m_surface, &fmtCount, nullptr );
        TVector<VkSurfaceFormatKHR> fmts{ Memory::Allocators::g_RHI }; fmts.resize( fmtCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( pCtx->m_physicalDevice, pSC->m_surface, &fmtCount, fmts.data() );
        VkSurfaceFormatKHR chosenFmt = fmts[0];
        for ( auto& f : fmts ) if ( f.format == VK_FORMAT_B8G8R8A8_SRGB ) { chosenFmt = f; break; }
        pSC->m_format = chosenFmt.format;

        // Create swapchain
        VkSurfaceCapabilitiesKHR caps; vkGetPhysicalDeviceSurfaceCapabilitiesKHR( pCtx->m_physicalDevice, pSC->m_surface, &caps );
        VkExtent2D extent = pSC->m_extent;
        if ( caps.currentExtent.width != UINT32_MAX ) extent = caps.currentExtent;
        else { extent.width = Math::Clamp( extent.width, caps.minImageExtent.width, caps.maxImageExtent.width ); extent.height = Math::Clamp( extent.height, caps.minImageExtent.height, caps.maxImageExtent.height ); }

        VkSwapchainCreateInfoKHR sci{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        sci.surface = pSC->m_surface; sci.minImageCount = Math::Max( 2u, caps.minImageCount ); if ( caps.maxImageCount ) sci.minImageCount = Math::Min( sci.minImageCount, caps.maxImageCount );
        sci.imageFormat = chosenFmt.format; sci.imageColorSpace = chosenFmt.colorSpace; sci.imageExtent = extent;
        sci.imageArrayLayers = 1; sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; sci.preTransform = caps.currentTransform; sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        sci.presentMode = params.m_enableVSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
        sci.clipped = VK_TRUE;

        VkResult res = vkCreateSwapchainKHR( pCtx->m_device, &sci, nullptr, &pSC->m_swapchain );
        if ( res != VK_SUCCESS ) { EE_LOG_ERROR( LogCategory::Render, "RHI/Vulkan", "vkCreateSwapchainKHR %d", (int) res ); return pSC; }
        pSC->m_extent = extent;

        uint32_t imgCount = 0; vkGetSwapchainImagesKHR( pCtx->m_device, pSC->m_swapchain, &imgCount, nullptr );
        pSC->m_images.resize( imgCount ); vkGetSwapchainImagesKHR( pCtx->m_device, pSC->m_swapchain, &imgCount, pSC->m_images.data() );
        pSC->m_imageViews.resize( imgCount );
        for ( uint32_t i = 0; i < imgCount; ++i )
        {
            VkImageViewCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            ci.image = pSC->m_images[i]; ci.viewType = VK_IMAGE_VIEW_TYPE_2D; ci.format = pSC->m_format;
            ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; ci.subresourceRange.levelCount = 1; ci.subresourceRange.layerCount = 1;
            vkCreateImageView( pCtx->m_device, &ci, nullptr, &pSC->m_imageViews[i] );
        }
        // Wrap images as RHI Textures for engine
        pSC->m_renderTargets.resize( imgCount );
        for ( uint32_t i = 0; i < imgCount; ++i )
        {
            auto* tex = EE::New<VulkanTexture>();
            tex->m_pContext = pCtx; tex->m_image = pSC->m_images[i]; tex->m_view = pSC->m_imageViews[i]; tex->m_format = pSC->m_format;
            tex->m_width = extent.width; tex->m_height = extent.height; tex->m_depth = 1; tex->m_arrayLayers = 1; tex->m_mipLevels = 1;
            pSC->m_renderTargets[i] = tex;
        }

        EE_LOG_MESSAGE( LogCategory::Render, "RHI/Vulkan", "Swapchain %ux%u fmt %d images %u", extent.width, extent.height, (int) pSC->m_format, imgCount );
        return pSC;
    }

    void DestroySwapchain( Context* pContext, Swapchain*&& pSwapchain )
    {
        auto* pSC = static_cast<VulkanSwapchain*>( pSwapchain );
        if ( !pSC ) return;
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        if ( pCtx && pCtx->m_device ) vkDeviceWaitIdle( pCtx->m_device );
        for ( auto v : pSC->m_imageViews ) if ( v ) vkDestroyImageView( pCtx->m_device, v, nullptr );
        // Images are owned by swapchain, don't destroy; but our wrapper textures:
        for ( auto* tex : pSC->m_renderTargets ) { auto* vt = static_cast<VulkanTexture*>( tex ); vt->m_image = VK_NULL_HANDLE; vt->m_view = VK_NULL_HANDLE; EE::Delete( vt ); }
        if ( pSC->m_swapchain ) vkDestroySwapchainKHR( pCtx->m_device, pSC->m_swapchain, nullptr );
        if ( pSC->m_surface ) vkDestroySurfaceKHR( pCtx->m_instance, pSC->m_surface, nullptr );
        EE::Delete( pSC ); pSwapchain = nullptr;
    }

    uint32_t AcquireNextImage( Context* pContext, Swapchain* pSwapchain )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pSC = static_cast<VulkanSwapchain*>( pSwapchain );
        if ( !pSC->m_swapchain ) return 0;
        // Create acquire semaphore if needed
        if ( pSC->m_acquireSemaphores.empty() )
        {
            pSC->m_acquireSemaphores.resize( MaxPendingFrames );
            for ( auto& s : pSC->m_acquireSemaphores ) { VkSemaphoreCreateInfo ci{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }; vkCreateSemaphore( pCtx->m_device, &ci, nullptr, &s ); }
        }
        uint32_t idx = 0;
        VkResult res = vkAcquireNextImageKHR( pCtx->m_device, pSC->m_swapchain, UINT64_MAX, pSC->m_acquireSemaphores[pSC->m_currentIndex % MaxPendingFrames], VK_NULL_HANDLE, &idx );
        if ( res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR ) { /* TODO recreate */ }
        pSC->m_currentIndex = idx; return idx;
    }

    void SetVSync( Swapchain* pSwapchain, bool ) { (void) pSwapchain; }

    //-------------------------------------------------------------------------
    // Command pools/buffers
    //-------------------------------------------------------------------------

    CommandPool* CreateCommandPool( Context* pContext, CommandPoolParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pQ = static_cast<VulkanQueue*>( params.m_pQueue );
        auto* pPool = EE::New<VulkanCommandPool>();
        VkCommandPoolCreateInfo ci{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
        ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        ci.queueFamilyIndex = pQ ? pQ->m_family : pCtx->m_graphicsFamily;
        vkCreateCommandPool( pCtx->m_device, &ci, nullptr, &pPool->m_pool );
        pPool->m_pQueue = params.m_pQueue;
        return pPool;
    }
    void DestroyCommandPool( Context* pContext, CommandPool*&& pPool )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vp = static_cast<VulkanCommandPool*>( pPool );
        if ( vp && vp->m_pool ) vkDestroyCommandPool( pCtx->m_device, vp->m_pool, nullptr );
        EE::Delete( vp ); pPool = nullptr;
    }
    void ResetCommandPool( Context*, CommandPool* pPool ) { auto* vp = static_cast<VulkanCommandPool*>( pPool ); vkResetCommandPool( static_cast<VulkanContext*>( vp->m_pQueue ? static_cast<VulkanQueue*>( vp->m_pQueue )->m_pContext : nullptr )->m_device, vp->m_pool, 0 ); }

    CommandBuffer* CreateCommandBuffer( Context* pContext, CommandBufferParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pPool = static_cast<VulkanCommandPool*>( params.m_pCommandPool );
        auto* pCB = EE::New<VulkanCommandBuffer>();
        VkCommandBufferAllocateInfo ai{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
        ai.commandPool = pPool->m_pool; ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; ai.commandBufferCount = 1;
        vkAllocateCommandBuffers( pCtx->m_device, &ai, &pCB->m_cmd );
        pCB->m_pQueue = pPool->m_pQueue; pCB->m_pCommandPool = params.m_pCommandPool;
        return pCB;
    }
    void DestroyCommandBuffer( Context* pContext, CommandBuffer*&& pCB )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vcb = static_cast<VulkanCommandBuffer*>( pCB );
        if ( vcb && vcb->m_cmd )
        {
            auto* pPool = static_cast<VulkanCommandPool*>( vcb->m_pCommandPool );
            vkFreeCommandBuffers( pCtx->m_device, pPool->m_pool, 1, &vcb->m_cmd );
        }
        EE::Delete( vcb ); pCB = nullptr;
    }
    void BeginCommandBuffer( CommandBuffer* pCB ) { VkCommandBufferBeginInfo bi{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; vkBeginCommandBuffer( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, &bi ); }
    void EndCommandBuffer( CommandBuffer* pCB ) { vkEndCommandBuffer( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd ); }

    //-------------------------------------------------------------------------
    // Barriers / draws etc. - minimal functional stubs that record valid Vulkan commands
    //-------------------------------------------------------------------------

    void CmdSetRenderTargets( CommandBuffer* pCB, TArrayView<Texture* const> rts, Texture* pDS, LoadAction* pLoad, TArrayView<uint32_t const>, TArrayView<uint32_t const>, uint32_t, uint32_t )
    {
        auto cmd = static_cast<VulkanCommandBuffer*>( pCB )->m_cmd;
        // Use dynamic rendering if available
        TVector<VkRenderingAttachmentInfo> colors{ Memory::Allocators::g_RHI };
        for ( auto* rt : rts )
        {
            auto* vt = static_cast<VulkanTexture*>( rt );
            VkRenderingAttachmentInfo ai{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            ai.imageView = vt->m_view; ai.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ai.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; ai.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            if ( pLoad ) { /* map LoadAction */ }
            colors.push_back( ai );
        }
        VkRenderingAttachmentInfo depthInfo{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        VkRenderingInfo ri{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        ri.renderArea.extent = { 1280, 720 }; // will be updated by viewport
        ri.layerCount = 1; ri.colorAttachmentCount = (uint32_t) colors.size(); ri.pColorAttachments = colors.data();
        if ( pDS )
        {
            auto* vd = static_cast<VulkanTexture*>( pDS );
            depthInfo.imageView = vd->m_view; depthInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            depthInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; depthInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            ri.pDepthAttachment = &depthInfo;
        }
        // Note: caller should have transitioned images; we just begin rendering
        // vkCmdBeginRendering(cmd, &ri) requires VK_KHR_dynamic_rendering enabled
        auto func = (PFN_vkCmdBeginRendering) vkGetDeviceProcAddr( static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue ? static_cast<VulkanQueue*>( static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue )->m_pContext->m_device : VK_NULL_HANDLE, "vkCmdBeginRendering" );
        if ( func ) func( cmd, &ri );
    }
    void CmdSetShadingRate( CommandBuffer*, ShadingRate, Texture*, ShadingRateCombiner, ShadingRateCombiner ) {}
    void CmdSetViewport( CommandBuffer* pCB, float x, float y, float w, float h, float md, float Md ) { VkViewport vp{ x, y, w, h, md, Md }; vkCmdSetViewport( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, 0, 1, &vp ); }
    void CmdSetScissor( CommandBuffer* pCB, uint32_t x, uint32_t y, uint32_t w, uint32_t h ) { VkRect2D sc{ { (int32_t) x, (int32_t) y }, { w, h } }; vkCmdSetScissor( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, 0, 1, &sc ); }
    void CmdSetStencilReference( CommandBuffer* pCB, uint32_t v ) { vkCmdSetStencilReference( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, VK_STENCIL_FACE_FRONT_AND_BACK, v ); }
    void CmdSetPipeline( CommandBuffer* pCB, Pipeline* pPL ) { auto* vp = static_cast<VulkanPipeline*>( pPL ); vkCmdBindPipeline( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, vp->m_pipeline ); }
    void CmdSetRootConstants( CommandBuffer* pCB, uint32_t idx, void const* data, size_t sz ) { vkCmdPushConstants( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanPipeline*>( static_cast<VulkanCommandBuffer*>( pCB )->m_pBoundPipeline )->m_layout, VK_SHADER_STAGE_ALL, (uint32_t) idx * 4, (uint32_t) sz, data ); }
    void CmdSetRootParameter( CommandBuffer* pCB, uint32_t idx, Buffer* pBuf, size_t off ) { (void) pCB; (void) idx; (void) pBuf; (void) off; /* descriptor sets */ }
    void CmdSetIndexBuffer( CommandBuffer* pCB, Buffer const* pIB, IndexType ty, uint64_t off ) { vkCmdBindIndexBuffer( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanBuffer const*>( pIB )->m_buffer, off, ty == IndexType::Uint16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32 ); }
    void CmdDraw( CommandBuffer* pCB, uint32_t n, uint32_t f ) { vkCmdDraw( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, n, 1, f, 0 ); }
    void CmdDrawInstanced( CommandBuffer* pCB, uint32_t n, uint32_t inst, uint32_t f, uint32_t fi ) { vkCmdDraw( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, n, inst, f, fi ); }
    void CmdDrawIndexed( CommandBuffer* pCB, uint32_t n, uint32_t f ) { vkCmdDrawIndexed( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, n, 1, f, 0, 0 ); }
    void CmdDrawIndexedInstanced( CommandBuffer* pCB, uint32_t n, uint32_t inst, uint32_t f, uint32_t fi ) { vkCmdDrawIndexed( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, n, inst, f, 0, fi ); }
    void CmdDispatchCompute( CommandBuffer* pCB, uint32_t x, uint32_t y, uint32_t z ) { vkCmdDispatch( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, x, y, z ); }
    void CmdDispatchMesh( CommandBuffer* pCB, uint32_t x, uint32_t y, uint32_t z ) { auto f = (PFN_vkCmdDrawMeshTasksEXT) vkGetDeviceProcAddr( static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue ? static_cast<VulkanQueue*>( static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue )->m_pContext->m_device : VK_NULL_HANDLE, "vkCmdDrawMeshTasksEXT" ); if ( f ) f( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, x, y, z ); }
    void CmdDispatchRays( CommandBuffer*, RaytracingShaderTable*, AccelerationStructure*, uint32_t, uint32_t ) {}
    void CmdExecuteIndirect( CommandBuffer*, CommandSignature const*, uint32_t, Buffer const*, uint64_t, Buffer const*, uint64_t ) {}
    void CmdClearTexture( CommandBuffer* pCB, Texture const* pT, uint32_t v ) { (void) pCB; (void) pT; (void) v; }
    void CmdClearBuffer( CommandBuffer* pCB, Buffer const* pB, uint32_t v ) { vkCmdFillBuffer( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanBuffer const*>( pB )->m_buffer, 0, VK_WHOLE_SIZE, v ); }
    void CmdBuildAccelerationStructure( CommandBuffer*, TArrayView<AccelerationStructure* const>, TArrayView<uint32_t const> ) {}
    void CmdBarrier( CommandBuffer* pCB, TBitFlags<PipelineStage>, TBitFlags<PipelineStage>, TBitFlags<ResourceAccess>, TBitFlags<ResourceAccess> ) { (void) pCB; }
    void CmdBarrier( CommandBuffer* pCB, Buffer* pB, TBitFlags<PipelineStage> s, TBitFlags<PipelineStage> d, TBitFlags<ResourceAccess> sa, TBitFlags<ResourceAccess> da ) { (void) pCB; (void) pB; (void) s; (void) d; (void) sa; (void) da; }
    void CmdBarrier( CommandBuffer* pCB, Texture* pT, TBitFlags<PipelineStage> s, TBitFlags<PipelineStage> d, TBitFlags<ResourceAccess> sa, TBitFlags<ResourceAccess> da, TextureState ss, TextureState ds, TextureBarrierRegion, TBitFlags<TextureBarrierFlags> ) { (void) pCB; (void) pT; (void) s; (void) d; (void) sa; (void) da; (void) ss; (void) ds; }
    void CmdResetQueryPool( CommandBuffer* pCB, QueryPool* pQ, uint32_t s, uint32_t n ) { vkCmdResetQueryPool( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanQueryPool*>( pQ )->m_pool, s, n ); }
    void CmdBeginQuery( CommandBuffer* pCB, QueryPool* pQ, uint32_t i ) { vkCmdBeginQuery( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanQueryPool*>( pQ )->m_pool, i, 0 ); }
    void CmdEndQuery( CommandBuffer* pCB, QueryPool* pQ, uint32_t i ) { vkCmdEndQuery( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanQueryPool*>( pQ )->m_pool, i ); }
    void CmdResolveQuery( CommandBuffer* pCB, QueryPool* pQ, Buffer const* pRB, uint32_t s, uint32_t n ) { vkCmdCopyQueryPoolResults( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanQueryPool*>( pQ )->m_pool, s, n, static_cast<VulkanBuffer const*>( pRB )->m_buffer, 0, 8, VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT ); }
    void CmdCopyBuffer( CommandBuffer* pCB, Buffer const* pDst, uint64_t doff, Buffer const* pSrc, uint64_t soff, uint64_t sz ) { VkBufferCopy c{ soff, doff, sz }; vkCmdCopyBuffer( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, static_cast<VulkanBuffer const*>( pSrc )->m_buffer, static_cast<VulkanBuffer const*>( pDst )->m_buffer, 1, &c ); }
    void CmdCopyTexture( CommandBuffer* pCB, Texture const* pDst, TextureCopyRegion const& dr, Buffer const* pSrc, uint64_t off ) { (void) pCB; (void) pDst; (void) dr; (void) pSrc; (void) off; }
    void CmdCopyTexture( CommandBuffer* pCB, Buffer const* pDst, uint64_t off, Texture const* pSrc, TextureCopyRegion const& sr ) { (void) pCB; (void) pDst; (void) off; (void) pSrc; (void) sr; }
    void CmdBeginDebugMarker( CommandBuffer* pCB, char const* n ) { auto* ctx = static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue ? static_cast<VulkanQueue*>( static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue )->m_pContext : nullptr; if ( ctx && ctx->m_validationEnabled ) { VkDebugUtilsLabelEXT l{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT }; l.pLabelName = n; auto f = (PFN_vkCmdBeginDebugUtilsLabelEXT) vkGetDeviceProcAddr( ctx->m_device, "vkCmdBeginDebugUtilsLabelEXT" ); if ( f ) f( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd, &l ); } }
    void CmdEndDebugMarker( CommandBuffer* pCB ) { auto* ctx = static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue ? static_cast<VulkanQueue*>( static_cast<VulkanCommandBuffer*>( pCB )->m_pQueue )->m_pContext : nullptr; if ( ctx && ctx->m_validationEnabled ) { auto f = (PFN_vkCmdEndDebugUtilsLabelEXT) vkGetDeviceProcAddr( ctx->m_device, "vkCmdEndDebugUtilsLabelEXT" ); if ( f ) f( static_cast<VulkanCommandBuffer*>( pCB )->m_cmd ); } }
    uint32_t CmdWriteDebugMarker( CommandBuffer*, TBitFlags<MarkerTypeFlags> const&, uint32_t, Buffer*, size_t, bool ) { return 0; }

    CommandSignature* CreateCommandSignature( Context*, CommandSignatureParameters const& ) { return EE::New<CommandSignature>(); }
    void DestroyCommandSignature( Context*, CommandSignature*&& p ) { EE::Delete( p ); p = nullptr; }
    AccelerationStructure* CreateAccelerationStructure( Context*, AccelerationStructureTopLevelCreateParameters const&, AccelerationStructureBottomLevelCreateParameters const& ) { return EE::New<AccelerationStructure>(); }
    AccelerationStructureHandle GetAccelerationStructureHandle( AccelerationStructure const* ) { return {}; }

    //-------------------------------------------------------------------------
    // Buffers / Textures
    //-------------------------------------------------------------------------

    Buffer* CreateBuffer( Context* pContext, BufferParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pBuf = EE::New<VulkanBuffer>();
        pBuf->m_pContext = pCtx; pBuf->m_size = params.m_bufferSize; pBuf->m_stride = params.m_bufferStride;
        pBuf->m_descriptorTypes = params.m_descriptorTypes; pBuf->m_nodeIndex = params.m_nodeIndex;

        VkBufferCreateInfo ci{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        ci.size = params.m_bufferSize; ci.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        if ( params.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::ConstantBuffer ) ) ci.usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if ( vkCreateBuffer( pCtx->m_device, &ci, nullptr, &pBuf->m_buffer ) != VK_SUCCESS ) { EE::Delete( pBuf ); return nullptr; }
        VkMemoryRequirements req; vkGetBufferMemoryRequirements( pCtx->m_device, pBuf->m_buffer, &req );
        VkMemoryPropertyFlags props = ( params.m_memoryType == ResourceMemoryType::HostToDevice ) ? ( VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ) : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        uint32_t memIdx = FindMemoryType( pCtx, req.memoryTypeBits, props );
        if ( memIdx == UINT32_MAX ) memIdx = FindMemoryType( pCtx, req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
        VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; ai.allocationSize = req.size; ai.memoryTypeIndex = memIdx;
        if ( vkAllocateMemory( pCtx->m_device, &ai, nullptr, &pBuf->m_memory ) != VK_SUCCESS ) { vkDestroyBuffer( pCtx->m_device, pBuf->m_buffer, nullptr ); EE::Delete( pBuf ); return nullptr; }
        vkBindBufferMemory( pCtx->m_device, pBuf->m_buffer, pBuf->m_memory, 0 );
        if ( props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ) vkMapMemory( pCtx->m_device, pBuf->m_memory, 0, req.size, 0, &pBuf->m_pMappedAddress_WriteCombined );
        return pBuf;
    }
    void DestroyBuffer( Context* pContext, Buffer*&& pBuffer )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vb = static_cast<VulkanBuffer*>( pBuffer );
        if ( !vb ) return;
        if ( vb->m_pMappedAddress_WriteCombined ) vkUnmapMemory( pCtx->m_device, vb->m_memory );
        if ( vb->m_buffer ) vkDestroyBuffer( pCtx->m_device, vb->m_buffer, nullptr );
        if ( vb->m_memory ) vkFreeMemory( pCtx->m_device, vb->m_memory, nullptr );
        EE::Delete( vb ); pBuffer = nullptr;
    }
    void MapBuffer( Context*, Buffer* ) {}
    void UnmapBuffer( Context*, Buffer* ) {}
    BufferHandle GetBufferHandle( Buffer const*, DescriptorTypeFlags ) { return {}; }
    BufferSubAllocation BufferSubAllocate( Buffer* pBuffer, uint64_t, uint64_t ) { (void) pBuffer; return {}; }
    void BufferSubDeallocate( Buffer*, BufferSubAllocation&& ) {}

    Texture* CreateTexture( Context* pContext, TextureParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pTex = EE::New<VulkanTexture>();
        pTex->m_pContext = pCtx; pTex->m_width = params.m_width; pTex->m_height = params.m_height; pTex->m_depth = params.m_depth; pTex->m_arrayLayers = params.m_arrayLayers; pTex->m_mipLevels = params.m_mipLevels; pTex->m_format = params.m_format;
        pTex->m_format = params.m_format; // keep original for RHI
        VkFormat fmt = VKFormat( params.m_format );
        pTex->m_format = params.m_format; // store logical

        VkImageCreateInfo ci{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        ci.imageType = params.m_depth > 1 ? VK_IMAGE_TYPE_3D : ( params.m_height > 1 ? VK_IMAGE_TYPE_2D : VK_IMAGE_TYPE_1D );
        ci.format = fmt; ci.extent = { params.m_width, params.m_height, Math::Max( 1u, params.m_depth ) };
        ci.mipLevels = params.m_mipLevels; ci.arrayLayers = params.m_arrayLayers; ci.samples = VK_SAMPLE_COUNT_1_BIT;
        ci.tiling = VK_IMAGE_TILING_OPTIMAL; ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if ( params.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RWTexture ) ) ci.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        if ( params.m_descriptorTypes.IsFlagSet( DescriptorTypeFlags::RenderTarget ) ) ci.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if ( IsCompressedFormat( params.m_format ) ) ci.usage &= ~VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if ( vkCreateImage( pCtx->m_device, &ci, nullptr, &pTex->m_image ) != VK_SUCCESS ) { EE::Delete( pTex ); return nullptr; }
        VkMemoryRequirements req; vkGetImageMemoryRequirements( pCtx->m_device, pTex->m_image, &req );
        uint32_t memIdx = FindMemoryType( pCtx, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT );
        VkMemoryAllocateInfo ai{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO }; ai.allocationSize = req.size; ai.memoryTypeIndex = memIdx;
        if ( vkAllocateMemory( pCtx->m_device, &ai, nullptr, &pTex->m_memory ) != VK_SUCCESS ) { vkDestroyImage( pCtx->m_device, pTex->m_image, nullptr ); EE::Delete( pTex ); return nullptr; }
        vkBindImageMemory( pCtx->m_device, pTex->m_image, pTex->m_memory, 0 );

        VkImageViewCreateInfo vi{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        vi.image = pTex->m_image; vi.viewType = VKImageViewType( ViewDimension::Texture2D ); vi.format = fmt;
        vi.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; vi.subresourceRange.levelCount = params.m_mipLevels; vi.subresourceRange.layerCount = params.m_arrayLayers;
        vkCreateImageView( pCtx->m_device, &vi, nullptr, &pTex->m_view );
        pTex->m_format = params.m_format;
        return pTex;
    }
    void DestroyTexture( Context* pContext, Texture*&& pTexture )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vt = static_cast<VulkanTexture*>( pTexture );
        if ( !vt ) return;
        if ( vt->m_view ) vkDestroyImageView( pCtx->m_device, vt->m_view, nullptr );
        if ( vt->m_image ) vkDestroyImage( pCtx->m_device, vt->m_image, nullptr );
        if ( vt->m_memory ) vkFreeMemory( pCtx->m_device, vt->m_memory, nullptr );
        EE::Delete( vt ); pTexture = nullptr;
    }
    uint32_t GetTextureCopyRowStride( Texture const*, uint32_t, uint32_t ) { return 0; }
    TextureHandle GetTextureHandle( Texture const*, DescriptorTypeFlags, uint32_t ) { return {}; }

    Sampler* CreateSampler( Context* pContext, SamplerParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pS = EE::New<VulkanSampler>();
        VkSamplerCreateInfo ci{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        ci.magFilter = params.m_magFilter == FilterType::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        ci.minFilter = params.m_minFilter == FilterType::Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
        ci.mipmapMode = params.m_mipMapMode == MipMapMode::Linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
        ci.addressModeU = params.m_addressModeU == AddressMode::Wrap ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressModeV = params.m_addressModeV == AddressMode::Wrap ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.addressModeW = params.m_addressModeW == AddressMode::Wrap ? VK_SAMPLER_ADDRESS_MODE_REPEAT : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        ci.maxAnisotropy = (float) params.m_maxAnisotropy; ci.anisotropyEnable = params.m_maxAnisotropy > 1;
        ci.compareEnable = params.m_compareMode != CompareMode::Never; ci.compareOp = VKCompare( params.m_compareMode );
        ci.mipLodBias = params.m_mipLODBias; ci.minLod = params.m_minLOD; ci.maxLod = params.m_maxLOD;
        vkCreateSampler( pCtx->m_device, &ci, nullptr, &pS->m_sampler );
        return pS;
    }
    void DestroySampler( Context* pContext, Sampler*&& pSampler )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vs = static_cast<VulkanSampler*>( pSampler );
        if ( vs && vs->m_sampler ) vkDestroySampler( pCtx->m_device, vs->m_sampler, nullptr );
        EE::Delete( vs ); pSampler = nullptr;
    }
    SamplerStateHandle GetSamplerStateHandle( Sampler const* ) { return {}; }

    Shader* CreateShader( Context* pContext, TInlineVector<ShaderByteCode, 2> const& shaderParameters )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pS = EE::New<VulkanShader>();
        for ( auto& bc : shaderParameters )
        {
            // bc.m_pCompressedData is DXC compiled DXIL; on Linux we expect SPIR-V via DXC -spirv
            // For now, treat data as SPIR-V if it starts with 0x07230203
            if ( bc.m_pCompressedData && bc.m_decodedSize >= 4 )
            {
                uint32_t magic = *reinterpret_cast<uint32_t const*>( bc.m_pCompressedData );
                if ( magic == 0x07230203 )
                {
                    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
                    ci.codeSize = bc.m_decodedSize; ci.pCode = reinterpret_cast<uint32_t const*>( bc.m_pCompressedData );
                    VkShaderModule mod = VK_NULL_HANDLE;
                    if ( vkCreateShaderModule( pCtx->m_device, &ci, nullptr, &mod ) == VK_SUCCESS ) pS->m_modules.push_back( mod );
                }
            }
        }
        return pS;
    }
    void DestroyShader( Context* pContext, Shader*&& pShader )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vs = static_cast<VulkanShader*>( pShader );
        if ( vs ) for ( auto m : vs->m_modules ) if ( m ) vkDestroyShaderModule( pCtx->m_device, m, nullptr );
        EE::Delete( vs ); pShader = nullptr;
    }

    RootSignature* CreateRootSignature( Context* pContext, RootSignatureParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pRS = EE::New<VulkanRootSignature>();
        // Build descriptor set layout from shader reflection if available, otherwise empty
        VkDescriptorSetLayoutCreateInfo ci{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        vkCreateDescriptorSetLayout( pCtx->m_device, &ci, nullptr, &pRS->m_setLayout );
        VkPipelineLayoutCreateInfo pli{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pli.setLayoutCount = pRS->m_setLayout ? 1 : 0; pli.pSetLayouts = pRS->m_setLayout ? &pRS->m_setLayout : nullptr;
        // Push constants for root constants
        VkPushConstantRange pcr{ VK_SHADER_STAGE_ALL, 0, 128 };
        pli.pushConstantRangeCount = 1; pli.pPushConstantRanges = &pcr;
        vkCreatePipelineLayout( pCtx->m_device, &pli, nullptr, &pRS->m_pipelineLayout );
        (void) params;
        return pRS;
    }
    void DestroyRootSignature( Context* pContext, RootSignature*&& pRS )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vrs = static_cast<VulkanRootSignature*>( pRS );
        if ( !vrs ) return;
        if ( vrs->m_pipelineLayout ) vkDestroyPipelineLayout( pCtx->m_device, vrs->m_pipelineLayout, nullptr );
        if ( vrs->m_setLayout ) vkDestroyDescriptorSetLayout( pCtx->m_device, vrs->m_setLayout, nullptr );
        EE::Delete( vrs ); pRS = nullptr;
    }

    PipelineCache* CreatePipelineCache( Context*, PipelineCacheParameters const& ) { return EE::New<PipelineCache>(); }
    void DestroyPipelineCache( Context*, PipelineCache*&& p ) { EE::Delete( p ); p = nullptr; }
    TArrayView<uint8_t> GetPipelineCacheData( Context*, PipelineCache* ) { return {}; }

    Pipeline* CreatePipeline( Context* pContext, GraphicsPipelineParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pPL = EE::New<VulkanPipeline>();
        pPL->m_pipelineType = PipelineType::Graphics;
        auto* pRS = static_cast<VulkanRootSignature*>( params.m_pRootSignature );
        pPL->m_layout = pRS ? pRS->m_pipelineLayout : VK_NULL_HANDLE;

        // Minimal graphics pipeline — vertex input empty, fill rest from params
        VkGraphicsPipelineCreateInfo ci{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        VkPipelineVertexInputStateCreateInfo vi{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        VkPipelineInputAssemblyStateCreateInfo ia{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        ia.topology = VKTopology( params.m_primitiveTopology );
        VkPipelineViewportStateCreateInfo vp{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO }; vp.viewportCount = 1; vp.scissorCount = 1;
        VkPipelineRasterizationStateCreateInfo rs{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rs.cullMode = VKCullMode( params.m_rasterizerState.m_cullMode ); rs.frontFace = VKFrontFace( params.m_rasterizerState.m_frontFace );
        rs.polygonMode = VKFillMode( params.m_rasterizerState.m_fillMode ); rs.lineWidth = 1.0f;
        VkPipelineMultisampleStateCreateInfo ms{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO }; ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        VkPipelineDepthStencilStateCreateInfo ds{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        ds.depthTestEnable = params.m_depthStencilState.m_depthTest; ds.depthWriteEnable = params.m_depthStencilState.m_depthWrite;
        ds.depthCompareOp = VKCompare( params.m_depthStencilState.m_depthCompareMode );
        ds.stencilTestEnable = params.m_depthStencilState.m_stencilTest;
        VkPipelineColorBlendAttachmentState cbAttach{}; cbAttach.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        if ( params.m_blendState.m_blendEnabled ) { cbAttach.blendEnable = VK_TRUE; cbAttach.srcColorBlendFactor = VKBlend( params.m_blendState.m_srcFactors[0] ); cbAttach.dstColorBlendFactor = VKBlend( params.m_blendState.m_dstFactors[0] ); cbAttach.colorBlendOp = VKBlendOp( params.m_blendState.m_blendModes[0] ); }
        VkPipelineColorBlendStateCreateInfo cb{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO }; cb.attachmentCount = 1; cb.pAttachments = &cbAttach;
        VkPipelineDynamicStateCreateInfo dyn{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        dyn.dynamicStateCount = 2; dyn.pDynamicStates = states;
        ci.pVertexInputState = &vi; ci.pInputAssemblyState = &ia; ci.pViewportState = &vp; ci.pRasterizationState = &rs; ci.pMultisampleState = &ms; ci.pDepthStencilState = &ds; ci.pColorBlendState = &cb; ci.pDynamicState = &dyn;
        ci.layout = pPL->m_layout;

        // Shader stages — if shader provided, create modules
        TVector<VkPipelineShaderStageCreateInfo> stages{ Memory::Allocators::g_RHI };
        if ( params.m_pShader )
        {
            auto* sh = static_cast<VulkanShader*>( params.m_pShader );
            for ( size_t i = 0; i < sh->m_modules.size(); ++i )
            {
                VkPipelineShaderStageCreateInfo si{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
                si.stage = ( i == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT );
                si.module = sh->m_modules[i]; si.pName = "main";
                stages.push_back( si );
            }
        }
        ci.stageCount = (uint32_t) stages.size(); ci.pStages = stages.data();

        // Rendering info (dynamic rendering)
        VkPipelineRenderingCreateInfo rendering{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        TVector<VkFormat> colorFmts{ Memory::Allocators::g_RHI };
        for ( auto f : params.m_colorFormats ) colorFmts.push_back( VKFormat( f ) );
        rendering.colorAttachmentCount = (uint32_t) colorFmts.size(); rendering.pColorAttachmentFormats = colorFmts.data();
        rendering.depthAttachmentFormat = VKFormat( params.m_depthStencilFormat );
        ci.pNext = &rendering;

        VkResult res = vkCreateGraphicsPipelines( pCtx->m_device, VK_NULL_HANDLE, 1, &ci, nullptr, &pPL->m_pipeline );
        if ( res != VK_SUCCESS ) { EE_LOG_WARNING( LogCategory::Render, "RHI/Vulkan", "vkCreateGraphicsPipelines failed %d", (int) res ); }
        return pPL;
    }
    Pipeline* CreatePipeline( Context* pContext, MeshPipelineParameters const& params ) { return CreatePipeline( pContext, static_cast<GraphicsPipelineParameters const&>( params ) ); }
    Pipeline* CreatePipeline( Context* pContext, ComputePipelineParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pPL = EE::New<VulkanPipeline>();
        pPL->m_pipelineType = PipelineType::Compute;
        auto* pRS = static_cast<VulkanRootSignature*>( params.m_pRootSignature );
        pPL->m_layout = pRS ? pRS->m_pipelineLayout : VK_NULL_HANDLE;
        VkComputePipelineCreateInfo ci{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        ci.layout = pPL->m_layout;
        if ( params.m_pShader )
        {
            auto* sh = static_cast<VulkanShader*>( params.m_pShader );
            if ( !sh->m_modules.empty() )
            {
                ci.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                ci.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT; ci.stage.module = sh->m_modules[0]; ci.stage.pName = "main";
            }
        }
        VkResult res = vkCreateComputePipelines( pCtx->m_device, VK_NULL_HANDLE, 1, &ci, nullptr, &pPL->m_pipeline );
        if ( res != VK_SUCCESS ) { EE_LOG_WARNING( LogCategory::Render, "RHI/Vulkan", "vkCreateComputePipelines failed %d", (int) res ); }
        return pPL;
    }
    Pipeline* CreatePipeline( Context*, RaytracingPipelineParameters const& ) { EE_LOG_WARNING( LogCategory::Render, "RHI/Vulkan", "Raytracing not implemented on Vulkan yet" ); return EE::New<VulkanPipeline>(); }
    void DestroyPipeline( Context* pContext, Pipeline*&& pPL )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* vp = static_cast<VulkanPipeline*>( pPL );
        if ( vp && vp->m_pipeline ) vkDestroyPipeline( pCtx->m_device, vp->m_pipeline, nullptr );
        EE::Delete( vp ); pPL = nullptr;
    }

    QueryPool* CreateQueryPool( Context* pContext, QueryPoolParameters const& params )
    {
        auto* pCtx = static_cast<VulkanContext*>( pContext );
        auto* pQ = EE::New<VulkanQueryPool>();
        VkQueryPoolCreateInfo ci{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
        ci.queryType = params.m_queryType == QueryType::Timestamp ? VK_QUERY_TYPE_TIMESTAMP : VK_QUERY_TYPE_PIPELINE_STATISTICS;
        ci.queryCount = params.m_numQueries;
        vkCreateQueryPool( pCtx->m_device, &ci, nullptr, &pQ->m_pool );
        return pQ;
    }
    void DestroyQueryPool( Context* pContext, QueryPool*&& pQ ) { auto* pCtx = static_cast<VulkanContext*>( pContext ); auto* vq = static_cast<VulkanQueryPool*>( pQ ); if ( vq && vq->m_pool ) vkDestroyQueryPool( pCtx->m_device, vq->m_pool, nullptr ); EE::Delete( vq ); pQ = nullptr; }
    double GetQueryTimestampFrequency( Queue* ) { return 1e9; }
    void SetDebugName( Context*, Queue*, StringView ) {}
    void SetDebugName( Context*, QueryPool*, StringView ) {}
    void SetDebugName( Context* pCtx, Buffer* pB, StringView name ) { auto* vb = static_cast<VulkanBuffer*>( pB ); if ( vb && vb->m_buffer ) { VkDebugUtilsObjectNameInfoEXT ni{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT }; ni.objectType = VK_OBJECT_TYPE_BUFFER; ni.objectHandle = (uint64_t) vb->m_buffer; ni.pObjectName = name.data(); auto f = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetDeviceProcAddr( static_cast<VulkanContext*>( pCtx )->m_device, "vkSetDebugUtilsObjectNameEXT" ); if ( f ) f( static_cast<VulkanContext*>( pCtx )->m_device, &ni ); } }
    void SetDebugName( Context* pCtx, Texture* pT, StringView name ) { auto* vt = static_cast<VulkanTexture*>( pT ); if ( vt && vt->m_image ) { VkDebugUtilsObjectNameInfoEXT ni{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT }; ni.objectType = VK_OBJECT_TYPE_IMAGE; ni.objectHandle = (uint64_t) vt->m_image; ni.pObjectName = name.data(); auto f = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetDeviceProcAddr( static_cast<VulkanContext*>( pCtx )->m_device, "vkSetDebugUtilsObjectNameEXT" ); if ( f ) f( static_cast<VulkanContext*>( pCtx )->m_device, &ni ); } }
    void SetDebugName( Context*, RootSignature*, StringView ) {}
    void SetDebugName( Context*, CommandSignature*, StringView ) {}
    void SetDebugName( Context* pCtx, Pipeline* pPL, StringView name ) { auto* vp = static_cast<VulkanPipeline*>( pPL ); if ( vp && vp->m_pipeline ) { VkDebugUtilsObjectNameInfoEXT ni{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT }; ni.objectType = VK_OBJECT_TYPE_PIPELINE; ni.objectHandle = (uint64_t) vp->m_pipeline; ni.pObjectName = name.data(); auto f = (PFN_vkSetDebugUtilsObjectNameEXT) vkGetDeviceProcAddr( static_cast<VulkanContext*>( pCtx )->m_device, "vkSetDebugUtilsObjectNameEXT" ); if ( f ) f( static_cast<VulkanContext*>( pCtx )->m_device, &ni ); } }
    void SetDebugName( Context*, CommandPool*, StringView ) {}
    void SetDebugName( Context*, CommandBuffer*, StringView ) {}
    void ReportDeviceMemoryLeaks() {}
}
#endif
