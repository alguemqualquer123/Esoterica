#if EE_PLATFORM_LINUX
#pragma once

#include "Game/_Module/GameModule.h"
#include "Engine/Engine.h"
#include "Base/Application/Platform/Application_Linux.h"

//-------------------------------------------------------------------------

namespace EE
{
    class StandaloneEngine final : public Engine
    {
        friend class ResourceEditorApplication;

    public:

        StandaloneEngine( TFunction<bool( EE::String const& error )>&& errorHandler );

        void RegisterTypes() override;
        void UnregisterTypes() override;

        virtual void PostInitialize() override;
        virtual void PreShutdown() override;

        virtual void ResizeMainWindow( Int2 newMainWindowDimensions ) override;

        #if EE_DEVELOPMENT_TOOLS
        virtual void CreateToolsUI() override;
        #endif

    private:

        Viewport* m_pGameViewport = nullptr;
    };

    //-------------------------------------------------------------------------

    class EngineApplication : public LinuxApplication
    {
    public:

        EngineApplication();

        virtual void ResizeMainWindow( Int2 const& newWindowSize ) override;
        virtual void ProcessSDLEvent( void* pSDLEvent ) override;

    protected:

        virtual bool Initialize( int32_t argc, char** argv ) override;
        virtual bool Shutdown() override;
        virtual bool ApplicationLoop() override;

    private:

        StandaloneEngine m_engine;
    };
}
#endif
