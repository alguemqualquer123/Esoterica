#if EE_PLATFORM_LINUX
#pragma once

#include "../EditorUI.h"
#include "Game/_Module/GameModule.h"
#include "GameTools/_Module/GameToolsModule.h"
#include "EngineTools/_Module/EngineToolsModule.h"
#include "Engine/Engine.h"
#include "Base/Application/Platform/Application_Linux.h"

//-------------------------------------------------------------------------

namespace EE
{
    class EditorEngine final : public Engine
    {
        friend class EditorApplication;

    public:

        EditorEngine( TFunction<bool( EE::String const& error )>&& errorHandler );

        virtual void RegisterTypes() override;
        virtual void UnregisterTypes() override;
        virtual void CreateToolsUI() override { m_pToolsUI = EE::New<EditorUI>(); }
        virtual void SetStartupMap( ResourceID const& mapID ) override;
    };

    //-------------------------------------------------------------------------

    class EditorApplication final : public LinuxApplication
    {
    public:

        EditorApplication();

    private:

        virtual bool Initialize( int32_t argc, char** argv ) override;
        virtual bool Shutdown() override;

        virtual bool FatalError( String const& error ) const override;

        virtual void GetBorderlessTitleBarInfo( Math::ScreenSpaceRectangle& outTitlebarRect, bool& isInteractibleWidgetHovered ) const override;
        virtual void ResizeMainWindow( Int2 const& newWindowSize ) override;
        virtual void ProcessSDLEvent( void* pSDLEvent ) override;

        virtual bool ApplicationLoop() override;

    private:

        EditorEngine m_engine;
    };
}
#endif
