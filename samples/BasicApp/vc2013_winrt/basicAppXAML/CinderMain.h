﻿#pragma once

// nb. this file was generated by the Window Store XAML template as DirectXPage, then modified for Cinder

/*

namespaces used

basicAppXAML::      all XAML content generated by Windows 8.1 Store/XAML template
                    pages are: App.xaml, CinderPage.xaml (this was the DirectXPage from the template)

DX::                DirectX helper and DeviceResources in the Common/ folder, generated by template

cinder::app::       contains CinderMain class that the application inherits from
cinder::dx::        contains the DirectX drawing methods, see dx.h

*/


// zv7
//#include "Common\StepTimer.h"
//#include "Common\DeviceResources.h"

// zv
// #include "Content\Sample3DSceneRenderer.h"
// #include "Content\SampleFpsTextRenderer.h"
// zv7
// #include "CinderBridge.h"

#include "cinder/Cinder.h"

#include "cinder/app/AppImplMswRendererDx.h"

#include "cinder/Vector.h"
#include "cinder/app/MouseEvent.h"
#include "cinder/app/KeyEvent.h"

// zv
using namespace Windows::UI::Core;

// zv7
// fwd decls
namespace DX {
    class StepTimer;
    class DeviceResources;

    /*
    
    interface IDeviceNotify
	{
		virtual void OnDeviceLost() = 0;
		virtual void OnDeviceRestored() = 0;
	};

    */
}

namespace cinder { namespace app {
// zv
// namespace basicAppXAML

    // zv8
    // class CinderMain : public DX::IDeviceNotify
    class CinderMain
    {
    public:
        CinderMain() : m_timer(nullptr) {}
        ~CinderMain();

        // Cinder methods (app will inherit these)
        virtual void mouseDrag(MouseEvent event) {}
        virtual void keyDown(KeyEvent event) {}
        virtual void update() {}
        virtual void draw() {}

        // setFullScreen: n/a: a Windows RT store app is always full screen
        //
        // multitouch: TBD
        // void enableMultiTouch( bool enable = true ) { mEnableMultiTouch = enable; }
        // 
        // cursor: does not apply
        //
        // framerate: changing it is useful, not implemented now
        //
        // quit: TBD, stop the RenderLoop
        //  nb. WinRT OS kills the app when it needs to, otherwise the app always runs
        //  nb. RenderLoop is automatically paused when the app is not visible, by XAML framework


        // Methods called by XAML via CinderPage
        void setup(const std::shared_ptr<DX::DeviceResources>& deviceResources);
        void CreateWindowSizeDependentResources();

        // pointer
        void StartTracking() {  m_tracking = true; }
        // zv
        // void TrackingUpdate(float x, float y) {}
        void TrackingUpdate(PointerEventArgs^ e);
        void StopTracking() { m_tracking = false; }
        bool IsTracking() { return m_tracking; }

        // rendering
        void StartRenderLoop();
        void StopRenderLoop();
        Concurrency::critical_section& GetCriticalSection() { return m_criticalSection; }

        // zv todo make this a smart ptr
        // std::shared_ptr<cinder::app::AppImplMswRendererDx> ren;
        cinder::app::AppImplMswRendererDx *ren;

        // share the DX/D3D/D2D objects with Cinder
        void shareWithCinder();

        // zv8
        // IDeviceNotify
        virtual void OnDeviceLost();
        virtual void OnDeviceRestored();

    private:        
        bool    m_tracking;
        bool    m_pipeline_ready;

        void ProcessInput();
        void Update();
        bool Render();

        // Cached pointer to device resources.
        std::shared_ptr<DX::DeviceResources> m_deviceResources;

        // TODO: Replace with your own content renderers.
        // zv
        // std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
        // std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;
        // std::unique_ptr<cinder::app::AppBasicXAML> m_sceneRenderer;

        // zv
        // ptr to base type
        // cinder::app::AppBasicXAML *m_sceneRenderer;

        Windows::Foundation::IAsyncAction^ m_renderLoopWorker;
        Concurrency::critical_section m_criticalSection;

        // Rendering loop timer.
        // zv7
        // DX::StepTimer m_timer;
        // std::unique_ptr<DX::StepTimer> m_timer;
        DX::StepTimer* m_timer;
    };

} }

// nb. a global base class ptr is declared and set, but the derived class is instantiated
#undef CINDER_APP_BASIC
#define CINDER_APP_BASIC( APP, RENDERER ) \
    CinderMain *app = new APP;

//	AppBasicXAML *app = new APP;													

