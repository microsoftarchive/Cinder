//////////////////////////////////////////////////////////////////////////
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//////////////////////////////////////////////////////////////////////////

// Interface to MediaCapture on WinRT
// adapted from MediaCapture sample/AdvancedCapture.xaml.cpp
// removed XAML dependencies

// #include "pch.h"

#include "MediaCaptureWinRT.h"

// #include  "app/winrt/CaptureLib.h"

#include <collection.h>
#include <vector>
#include <utility>

#include <ppltasks.h>
#include <ppl.h>
#include <agile.h>

#include <mfobjects.h>
#include <mfreadwrite.h>
// #include <mfidl.h>

#include <wrl\implements.h>
#include <comutil.h>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::System;
// using namespace Windows::UI::Xaml::Media::Imaging;

using namespace Windows::Devices::Enumeration;
using namespace Windows::Media;
using namespace Windows::Media::Capture;

using namespace Concurrency;
using namespace std;

using namespace Microsoft::WRL::Details;


namespace MediaWinRT
{
#if 0
    // call CaptureLib - Media Foudnation using COM
    // won't work because many COM interfaces are switched off for Windows Store Apps
    // maybe some can be used, but we need to make top level calls from WinRT interfaces first
    void MediaCaptureWinRT::start()
    {
        CaptureLib::start();
    }

#endif

// incomplete capture using a custom media sink
// status: frames are not grabbed in the sink, frames are not passed to Cinder
#if 1
    void MediaCaptureWinRT::start()
    {
        try
        {
            create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
                .then([this](task<DeviceInformationCollection^> findTask)
            {
                m_devInfoCollection = findTask.get();

                if (m_devInfoCollection->Size == 0 ||
                    m_selectedVideoDeviceIndex > m_devInfoCollection->Size) return;

                // see http://msdn.microsoft.com/en-us/library/windows/apps/windows.media.capture.mediacaptureinitializationsettings.aspx

                auto settings = ref new Windows::Media::Capture::MediaCaptureInitializationSettings();
                auto chosenDevInfo = m_devInfoCollection->GetAt(m_selectedVideoDeviceIndex);
                auto name = chosenDevInfo->Name;
                settings->VideoDeviceId = chosenDevInfo->Id;
                settings->StreamingCaptureMode = Windows::Media::Capture::StreamingCaptureMode::Video;
                
                create_task(m_mediaCaptureMgr->InitializeAsync(settings))
                    .then([this](task<void> initTask)
                {
                    initTask.get();
                    auto mediaCapture = m_mediaCaptureMgr.Get();
                    mediaCapture->SetRecordRotation(Windows::Media::Capture::VideoRotation::None);

                    MediaEncodingProfile^ recordProfile = nullptr;
                    recordProfile = MediaEncodingProfile::CreateMp4(Windows::Media::MediaProperties::VideoEncodingQuality::Auto);
 
#if 1 
                    IMediaExtension^ customMediaSink = nullptr;

                    Microsoft::WRL::ComPtr<IMediaExtension> cpME;
                    MakeAndInitialize<IMediaExtension>(&cpME);

                    // see http://msdn.microsoft.com/en-us/library/windows/apps/hh700855.aspx
                    create_task(m_mediaCaptureMgr->StartRecordToCustomSinkAsync(recordProfile, customMediaSink));

                    // Microsoft::WRL::ComPtr
                    /*
                    MakeAndInitialize<MediaSink>(&_mediaSink, audioPropsABI.Get(), videoPropsABI.Get(), audioSampleHandler, videoSampleHandler);

                    auto _mediaExtension = reinterpret_cast<IMediaExtension^>(static_cast<ABI::Windows::Media::IMediaExtension*>(_mediaSink.Get()));
                    */
#endif
                    // use other form where we create explicitly and can initialize
                    // create_task(m_mediaCaptureMgr->StartRecordToCustomSinkAsync(recordProfile, 
                        // "CaptureMediaSink.CaptureSink", nullptr));

// notes
#if 0
// MediaCapture.StartRecordToStreamAsync | startRecordToStreamAsync method
// http://msdn.microsoft.com/en-us/library/windows/apps/hh700868.aspx

// BitmapEncoder.CreateAsync(Guid, IRandomAccessStream) | createAsync(Guid, IRandomAccessStream) method
// http://msdn.microsoft.com/en-us/library/windows/apps/br226212.aspx

// auto stream = ref new IRandomAccessStream();

create_task(m_mediaCaptureMgr->StartRecordToStreamAsync(
    recordProfile, stream
    ));

// try using a bytestream
// see http://social.msdn.microsoft.com/Forums/windowsapps/en-US/49bffa74-4e84-4fd6-9d67-42e8385611b8/video-sinkwriter-in-metro-app?forum=winappswithnativecode

// MFCreateSinkWriterFromURL
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd388105(v=vs.85).aspx                    

// MFCreateSourceReaderFromMediaSource
// http://msdn.microsoft.com/en-us/library/windows/desktop/dd388108(v=vs.85).aspx

// MFCreateSinkWriterFromURL(nullptr, &bstream, &attr, &sinkwriter);

// IMFByteStream interface
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms698720(v=vs.85).aspx


// todo ?

// Microsoft::WRL::ComPtr<CSink> sink;
// MakeAndInitialize<...>( ... )

reference:

// Microsoft::WRL::ComPtr<IMFMediaType> videoMT;

// won't compile:
// ABI::Windows::Media::MediaProperties::IVideoEncodingProperties* videoProps;

// from CaptureReaderSharedState.cpp
CHK(MakeAndInitialize<MediaSink>(&_mediaSink, audioPropsABI.Get(), videoPropsABI.Get(), audioSampleHandler, videoSampleHandler));
_mediaExtension = reinterpret_cast<IMediaExtension^>(static_cast<ABI::Windows::Media::IMediaExtension*>(_mediaSink.Get()));

// from MediaSink.h
Microsoft::WRL::ComPtr<IMFMediaType> videoMT;
if (videoProps != nullptr)
{
CHK_RETURN(MFCreateMediaTypeFromProperties(videoProps, &videoMT));
CHK_RETURN(Microsoft::WRL::Details::MakeAndInitialize<MediaStreamSink>(&_videoStreamSink, this, c_videoStreamSinkId, videoMT.Get(), videoSampleHandler));
}

#endif

                });
            });
        }
        catch (Exception ^e)
        {
            // todo: handle exception
            // in CaptureImplWinRT, see:
            // throw CaptureExcInitFail();
        }
    }

#endif

#if 0

// working capture using an effect (based on grayscale)
// status: frames are not grabbed in effect, frames are not passed to Cinder

    void MediaCaptureWinRT::start()
    {
        try
        {
            create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
                .then([this](task<DeviceInformationCollection^> findTask)
            {
                m_devInfoCollection = findTask.get();

                if (m_devInfoCollection->Size == 0 ||
                    m_selectedVideoDeviceIndex > m_devInfoCollection->Size) return;

                // see http://msdn.microsoft.com/en-us/library/windows/apps/windows.media.capture.mediacaptureinitializationsettings.aspx

                auto settings = ref new Windows::Media::Capture::MediaCaptureInitializationSettings();
                auto chosenDevInfo = m_devInfoCollection->GetAt(m_selectedVideoDeviceIndex);
                auto name = chosenDevInfo->Name;
                settings->VideoDeviceId = chosenDevInfo->Id;

                create_task(m_mediaCaptureMgr->InitializeAsync(settings))
                    .then([this](task<void> initTask)
                {
                    initTask.get();
                    auto mediaCapture = m_mediaCaptureMgr.Get();
                    mediaCapture->SetRecordRotation(Windows::Media::Capture::VideoRotation::None);

                    // pass in the Media Extension Communication object property set "MEcomm"
                    //    create_task(m_mediaCaptureMgr->AddEffectAsync(Windows::Media::Capture::MediaStreamType::VideoPreview, "GrayscaleTransform.GrayscaleEffect", nullptr))
                    // MEcomm
                    //
                    // "CaptureMediaExtension.CaptureEffect"
                    // "GrayscaleTransform.GrayscaleEffect"
                    //
                    // note: Extension will be needed in the appxmanifest, see
                    // http://msdn.microsoft.com/en-us/library/windows/apps/br211479.aspx
                    //                   
                    create_task(mediaCapture->AddEffectAsync(
                        Windows::Media::Capture::MediaStreamType::VideoRecord,
                        "CaptureMediaExtension.CaptureEffect", nullptr))
                        .then([this](task<void> effectTask)
                    {

                        String ^fileName;
                        fileName = "cinder_video.mp4";

                        create_task(KnownFolders::VideosLibrary->CreateFileAsync(fileName,
                            Windows::Storage::CreationCollisionOption::GenerateUniqueName))
                            .then([this](task<StorageFile^> fileTask)
                        {
                            this->m_recordStorageFile = fileTask.get();

                            MediaEncodingProfile^ recordProfile = nullptr;
                            recordProfile = MediaEncodingProfile::CreateMp4
                                (Windows::Media::MediaProperties::VideoEncodingQuality::Auto);

                            return m_mediaCaptureMgr->StartRecordToStorageFileAsync(recordProfile, this->m_recordStorageFile);
                        });
                    });
                });
            });
        }
        catch (Exception ^e)
        {
            // todo: handle exception
            // in CaptureImplWinRT, see:
            // throw CaptureExcInitFail();
        }
    }
#endif

    void MediaCaptureWinRT::stop()
    {
        create_task(m_mediaCaptureMgr->StopRecordAsync())
            .then([this](task<void> recordTask)
        {
            // zv off - so that the video can be played to verify that the effect is working
            // create_task( m_recordStorageFile->DeleteAsync());
        });
    }

    MediaCaptureWinRT::MediaCaptureWinRT()
    {
        m_selectedVideoDeviceIndex = 0;

        m_mediaCaptureMgr = ref new Windows::Media::Capture::MediaCapture();

        // zv todo: Media Extension init
    }

    void MediaCaptureWinRT::GetVideoCamerasAsync(GetMediaDevicesDelegate^ func)
    {
        GetMediaDevicesDelegate^ delegate = func;

        create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
            .then([delegate](task<DeviceInformationCollection^> findTask)
        {
            Platform::Array<VideoDeviceInfo^>^ devices = ref new Platform::Array<VideoDeviceInfo^>(0);
            try
            {
                auto devInfoCollection = findTask.get();
                devices = ref new Platform::Array<VideoDeviceInfo^>(devInfoCollection->Size);
                for (size_t i = 0; i < devInfoCollection->Size; i++)
                {
                    auto devInfo = devInfoCollection->GetAt(i);
                    auto location = devInfo->EnclosureLocation;
                    bool isFrontFacing = false;
                    bool isBackFacing = false;
                    if ( location != nullptr ) 
                    {
                        isFrontFacing = (location->Panel == Windows::Devices::Enumeration::Panel::Front);
                        isBackFacing = (location->Panel == Windows::Devices::Enumeration::Panel::Back);
                    }   

                    // allocate VideoDeviceInfo object before use
                    devices[i] = ref new VideoDeviceInfo();
					
                    devices[i]->devName = devInfo->Name;
                    devices[i]->isFrontFacing = isFrontFacing;
                    devices[i]->isBackFacing = isBackFacing;
                }
            }
            catch (Exception ^e)
            {
                // todo: handle exception
                // in CaptureImplWinRT, see:
                // throw CaptureExcInitFail();
            }

            delegate(devices);

        });
    }

// reference notes
#if 0
    bool MediaCaptureWinRT::startDevices(int webcam, int mic)
    {
        selectedVideoDeviceIndex = webcam;
//        selectedMicrophoneDeviceIndex = mic;

        auto mediaCapture = ref new Windows::Media::Capture::MediaCapture();
        m_mediaCaptureMgr = mediaCapture;

        auto settings = ref new Windows::Media::Capture::MediaCaptureInitializationSettings();

        auto chosenDevInfo = m_devInfoCollection->GetAt(selectedVideoDeviceIndex);
        settings->VideoDeviceId = chosenDevInfo->Id;

//        auto chosenMicrophoneInfo = m_microPhoneInfoCollection->GetAt(selectedMicrophoneDeviceIndex);
//        settings->AudioDeviceId = chosenMicrophoneInfo->Id;

        // task 1 InitializeAsync
        create_task(mediaCapture->InitializeAsync(settings))
            .then([this](task<void> initTask)
        {
            try
            {
                initTask.get();

                auto mediaCapture = m_mediaCaptureMgr.Get();
                if (mediaCapture->MediaCaptureSettings->VideoDeviceId != nullptr && mediaCapture->MediaCaptureSettings->AudioDeviceId != nullptr)
                {
                    // mediaCapture->RecordLimitationExceeded += ref new Windows::Media::Capture::RecordLimitationExceededEventHandler(this, &AdvancedCapture::RecordLimitationExceeded);
                    // mediaCapture->Failed += ref new Windows::Media::Capture::MediaCaptureFailedEventHandler(this, &AdvancedCapture::Failed);

#if 0
                    // optional / extra feature
                    if (m_mediaCaptureMgr->MediaCaptureSettings->ConcurrentRecordAndPhotoSupported)
                    {
                        // prepare lowlag photo, then prepare lowlag record
                        create_task(m_mediaCaptureMgr->PrepareLowLagPhotoCaptureAsync(ImageEncodingProperties::CreateJpeg()))
                            .then([this](LowLagPhotoCapture ^photoCapture)
                        {
                            // PrepareLowLagRecordAsync();
                        });
                    }
#endif
                }
                else
                {
                    // error: no video/audio device found
                }
            }
            catch (Exception ^e)
            {
                ShowExceptionMessage(e);
            }
        });

        return true;
    }
#endif

}
