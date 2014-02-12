﻿//
// CinderMain: a base class for Cinder Applications using XAML
//
// nb. this file was generated by the Windows 8.1 Store XAML template as DirectXPage, then modified for Cinder

// The copyright in this software is being made available under the BSD License, included below. 
// This software may be subject to other third party and contributor rights, including patent rights, 
// and no such rights are granted under this license.
//
// Copyright (c) 2013, Microsoft Open Technologies, Inc. 
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, 
// are permitted provided that the following conditions are met:
//
// - Redistributions of source code must retain the above copyright notice, 
//   this list of conditions and the following disclaimer.
// - Redistributions in binary form must reproduce the above copyright notice, 
//   this list of conditions and the following disclaimer in the documentation 
//   and/or other materials provided with the distribution.
// - Neither the name of Microsoft Open Technologies, Inc. nor the names of its contributors 
//   may be used to endorse or promote products derived from this software 
//   without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, 
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// pch off
// #include "pch.h"
#include <wrl.h>
#include <wrl/client.h>
#include <d3d11_2.h>
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <wincodec.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>
#include <collection.h>


#include "app/winrt/xaml/CinderMain.h"
#include "app/winrt/xaml/Common/DirectXHelper.h"
#include "app/winrt/xaml/Common/StepTimer.h"
#include "app/winrt/xaml/Common/DeviceResources.h"

#include "cinder/app/AppBasic.h"
#include "cinder/app/AppImplWinRTBasic.h"

// this function is defined by the application start macro
extern int mainXAML();

using namespace Windows::UI::Xaml;


using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Platform;
using namespace Concurrency;

namespace DX {
    // This internal class relays DeviceLost and DeviceRestored events from 
    // class DeviceResources to class CinderMain, so that CinderMain
    // need not inherit from IDeviceNotify.
    class DeviceRelay final : public DX::IDeviceNotify
    {
    public:
        DeviceRelay::DeviceRelay( cinder::app::CinderMain * inst, 
            const std::shared_ptr<DX::DeviceResources>& deviceResources) : m_inst(inst)
        { 
            deviceResources->RegisterDeviceNotify(this);
        }

        // relay events to CinderMain
        // these implement the abstract methods in IDeviceNotify
        virtual void OnDeviceLost()     {   m_inst->OnDeviceLost();      }
        virtual void OnDeviceRestored() {   m_inst->OnDeviceRestored();  } 
    private:
        cinder::app::CinderMain * m_inst;
    };
}


namespace cinder {
    namespace app {

        CinderMain* CinderMain::sInstance;

        // Loads and initializes application assets when the application is loaded.
        void CinderMain::setup(const std::shared_ptr<DX::DeviceResources>& deviceResources)
        {
            m_deviceResources = deviceResources;

            // Register to be notified if the Device is lost or recreated
            m_relay = new DX::DeviceRelay( this, m_deviceResources );

            // instantiate the Cinder ::app
			// mainXAML() is declared by the CINDER_APP_BASIC macro in AppBasic.h
            mainXAML();

            // set singleton ptr
            CinderMain::sInstance = this;

            // optional: frames per second renderer (see SampleFpsTextRenderer in the XAML template)
            // m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

            // timer for steady and controllable drawing rate (distinct from the frame rate)
            m_timer = new DX::StepTimer;
            // optional: change the timer settings if you want something other than the default variable timestep mode.
            // e.g. for 60 FPS fixed timestep update logic, call:
            //
            // m_timer->SetFixedTimeStep(true);
            // m_timer->SetTargetElapsedSeconds(1.0 / 60);
            //

            // call AppImplWinRTBasic::runReady() manually
            // runReady:
            //      associates the renderer class with the app class, in Window::setRenderer
            //      calls the setup() of the application
            //      for WinRT (non-XAML) it then starts the renderer loop
            //      for XAML, it returns early and returns control here to the XAML framework
            //
            // Before we can start the XAML render loop, Windows OS/XAML will fire some events 
            // into CinderPage.xaml.cpp. 
            //
            auto impl = AppBasic::get()->getImpl();
            auto win = ::Window::Current->CoreWindow;
            impl->runReady( win );

            // get Cinder renderer for DirectX, created by RendererDx::setup()
            //  nb. Cinder will perform shader & lighting setup, and drawing via dx::
            //  initialization & device management is handled by the XAML framework
            //
            mRenderer = ((app::RendererDx*)(&*app::App::get()->getRenderer()))->mImpl;

            // call the app's content initialization setup(); this method can be overloaded.
            shareWithCinder();

            // setup Cinder's shaders and lighting if needed
            if (!m_pipeline_ready) {
                mRenderer->setupPipeline();
                m_pipeline_ready = true;
                m_resize_needed = true;
            }

            AppBasic::get()->setup();

            // emit resize after pipeline setup
        }

        CinderMain::~CinderMain()
        {
            // Deregister device notification
            m_deviceResources->RegisterDeviceNotify(nullptr);

            delete m_timer;
            delete m_relay;
        }
        
        // Updates application state when the window size changes (e.g. device orientation change)
        void CinderMain::CreateWindowSizeDependentResources()
        {
            // optional: add call to the size-dependent initialization of your app's content.
            m_resize_needed = true;
        }

        void CinderMain::StartRenderLoop()
        {
            // If the animation render loop is already running then do not start another thread.
            if (m_renderLoopWorker != nullptr && m_renderLoopWorker->Status == AsyncStatus::Started)
            {
                return;
            }

            // Create a task that will be run on a background thread.
            auto workItemHandler = ref new WorkItemHandler([this](IAsyncAction ^ action)
            {
                // Calculate the updated frame and render once per vertical blanking interval.
                while (action->Status == AsyncStatus::Started)
                {
                    critical_section::scoped_lock lock(m_criticalSection);
                    Update();
                    if (Render())
                    {
                        m_deviceResources->Present();
                    }
                }
            });

            // Run task on a dedicated high priority background thread.
            m_renderLoopWorker = ThreadPool::RunAsync(workItemHandler, WorkItemPriority::High, WorkItemOptions::TimeSliced);
        }

        void CinderMain::StopRenderLoop()
        {
            m_renderLoopWorker->Cancel();
        }

        // Updates the application state once per frame.
        void CinderMain::Update()
        {
            ProcessInput();

            // Update scene objects.
            // nb. this is a C++ 11 lambda function using the instance of DX::StepTimer
            m_timer->Tick([&]()
            {
                // XAML template says here: "TODO: Replace this with your app's content update functions."
                // call to overloaded Cinder update()
                AppBasic::get()->update();

                // optional: if you are using the frame rate display
                // m_fpsTextRenderer->Update(m_timer);
            });
        }


        // Renders the current frame according to the current application state.
        // Returns true if the frame was rendered and is ready to be displayed.
        bool CinderMain::Render()
        {
            // Don't try to render anything before the first Update.
            if (m_timer->GetFrameCount() == 0)
            {
                return false;
            }

            auto context = m_deviceResources->GetD3DDeviceContext();

            // Reset the viewport to target the whole screen.
            auto viewport = m_deviceResources->GetScreenViewport();
            context->RSSetViewports(1, &viewport);

            // Reset render targets to the screen.
            ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
            context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

            // optional but common: clearing the framebuffer will be done in the app
            // context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::CornflowerBlue);

            // Clear the depth stencil view.
            context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);


            // Render the scene objects:


            // set Cinder's pointers to the DX/D3D interfaces created here in the XAML framework
            shareWithCinder();

            // emit resize, refer to AppImplWinRTBasic::runReady
            auto win = AppBasic::get()->getImpl()->getWindow();
            if (m_resize_needed) {
                m_resize_needed = false;
                win->emitResize();
            }

            // setup Cinder's 3D camera and projection
            float w = m_deviceResources->GetOutputSize().Width;
            float h = m_deviceResources->GetOutputSize().Height;
            mRenderer->setupCamera(w, h);

            // calls to overloaded Cinder app draw() method
            AppBasic::get()->draw();

            // optional: display the frame rate
            // m_fpsTextRenderer->Render();

            return true;
        }

        void CinderMain::shareWithCinder()
        {
            // setup for Cinder::dx
            auto device = m_deviceResources->GetD3DDevice();
            auto features = m_deviceResources->GetDeviceFeatureLevel();
            auto context = m_deviceResources->GetD3DDeviceContext();
            auto viewport = m_deviceResources->GetScreenViewport();
            auto target = m_deviceResources->GetBackBufferRenderTargetView();
            auto stencil = m_deviceResources->GetDepthStencilView();

            mRenderer->md3dDevice = device;
            mRenderer->mFeatureLevel = features;
            mRenderer->mDeviceContext = context;
            mRenderer->mMainFramebuffer = target;
            mRenderer->mDepthStencilView = stencil;
        }

        // Notifies renderers that device resources need to be released.
        void CinderMain::OnDeviceLost()
        {
            m_pipeline_ready = false;

            // optional: if using frame rate display, release it
            // m_fpsTextRenderer->ReleaseDeviceDependentResources();
        }

        // Notifies renderers that device resources may now be recreated.
        void CinderMain::OnDeviceRestored()
        {
            // optional: if using frame rate display, create it
            // m_fpsTextRenderer->CreateDeviceDependentResources();

            CreateWindowSizeDependentResources();
        }

        void CinderMain::OnPointerPressed(PointerEventArgs^ args)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            std::shared_ptr<PointerEvent> e(new PointerEvent(PointerEventType::PointerPressed, args));
            mInputEvents.push(e);
        }

        void CinderMain::ProcessPointerPressed(PointerEventArgs^ args)
        {
            AppBasic::get()->getImpl()->handlePointerDown(args);
        }

        void CinderMain::OnPointerMoved(PointerEventArgs^ args)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            std::shared_ptr<PointerEvent> e(new PointerEvent(PointerEventType::PointerMoved, args));
            mInputEvents.push(e);
        }

        void CinderMain::ProcessPointerMoved(PointerEventArgs^ args)
        {
            AppBasic::get()->getImpl()->handlePointerMoved(args);
        }

        void CinderMain::OnPointerReleased(PointerEventArgs^ args)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            std::shared_ptr<PointerEvent> e(new PointerEvent(PointerEventType::PointerReleased, args));
            mInputEvents.push(e);
        }

        void CinderMain::ProcessPointerReleased(PointerEventArgs^ args)
        {
            AppBasic::get()->getImpl()->handlePointerUp(args);
        }

        void CinderMain::OnKeyDown(KeyEventArgs^ args)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            std::shared_ptr<KeyboardEvent> e(new KeyboardEvent(XamlKeyEvent::KeyDown, args));
            mInputEvents.push(e);
         }

        void CinderMain::ProcessOnKeyDown(KeyEventArgs^ args)
        {
            AppBasic::get()->getImpl()->handleKeyDown(args);
        }

        void CinderMain::OnKeyUp(KeyEventArgs^ args)
        {
            std::lock_guard<std::mutex> guard(mMutex);
            std::shared_ptr<KeyboardEvent> e(new KeyboardEvent(XamlKeyEvent::KeyUp, args));
            mInputEvents.push(e);
        }

        void CinderMain::ProcessOnKeyUp(KeyEventArgs^ args)
        {
            AppBasic::get()->getImpl()->handleKeyUp(args);
        }

        void CinderMain::ProcessInput()
        {
            std::lock_guard<std::mutex> guard(mMutex);

            while (!mInputEvents.empty())
            {
                InputEvent* e = mInputEvents.front().get();
                e->execute(this);
                mInputEvents.pop();
            }
        }


        void CinderMain::OnPointerWheelChanged(PointerEventArgs^ args)
        {
            // not implemented

        }
    }
} // end namespaces
