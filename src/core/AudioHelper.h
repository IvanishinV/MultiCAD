#pragma once

#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <atlbase.h>
#include <format>

class AudioHelper
{
public:
    static float EnsureMaxVolume()
    {
        float volume{ 0.0f };

        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
            return 0.0f;
        const bool needUninit = SUCCEEDED(hr);

        CComPtr<IMMDeviceEnumerator> en;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&en));
        if (FAILED(hr))
        {
            if (needUninit)
                CoUninitialize();
            return 0.0f;
        }

        CComPtr<IMMDevice> dev;
        hr = en->GetDefaultAudioEndpoint(eRender, eConsole, &dev);
        if (FAILED(hr) || !dev)
        {
            if (needUninit)
                CoUninitialize();
            return 0.0f;
        }

        CComPtr<IAudioSessionManager2> mgr;
        hr = dev->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&mgr);
        if (FAILED(hr) || !mgr)
        {
            if (needUninit)
                CoUninitialize();
            return 0.0f;
        }

        DWORD pid = GetCurrentProcessId();

        const auto start = std::chrono::steady_clock::now();
        constexpr auto timeout = std::chrono::seconds(5);
        constexpr auto interval = std::chrono::milliseconds(200);
        for (; std::chrono::steady_clock::now() - start < timeout; std::this_thread::sleep_for(interval))
        {
            CComPtr<IAudioSessionEnumerator> se;
            mgr->GetSessionEnumerator(&se);
            if (!se)
                continue;

            int count{ 0 };
            se->GetCount(&count);

            bool found{ false };

            for (int i = 0; i < count; ++i)
            {
                CComPtr<IAudioSessionControl> ctrl;
                se->GetSession(i, &ctrl);
                if (!ctrl)
                    continue;

                CComPtr<IAudioSessionControl2> ctrl2;
                ctrl->QueryInterface(IID_PPV_ARGS(&ctrl2));
                if (!ctrl2)
                    continue;

                DWORD spid = 0;
                ctrl2->GetProcessId(&spid);

                if (spid != pid)
                    continue;

                CComPtr<ISimpleAudioVolume> vol;
                ctrl2->QueryInterface(IID_PPV_ARGS(&vol));
                if (vol)
                {
                    hr = vol->GetMasterVolume(&volume);

                    if (SUCCEEDED(hr) && volume < 0.001f)
                        vol->SetMasterVolume(1.0f, nullptr);
                }

                found = true;
                break;
            }

            if (found)
                break;
        }

        if (needUninit)
            CoUninitialize();

        return volume;
    }
};
