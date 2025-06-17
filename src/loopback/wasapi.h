#pragma once
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <iostream>
#include <Functiondiscoverykeys_devpkey.h>

class c_Wasapi {
public:
    IMMDeviceEnumerator* pEnumerator;
    IMMDevice* pDevice;
    IAudioClient* pAudioClient;
    IAudioCaptureClient* pCaptureClient;
    WAVEFORMATEX* pwfx;
    bool initialized;

    c_Wasapi() :
        pEnumerator(nullptr), pDevice(nullptr),
        pAudioClient(nullptr), pCaptureClient(nullptr),
        pwfx(nullptr), initialized(false) {
    }

    ~c_Wasapi() {
        Cleanup();
    }

    bool Init(int deviceIndex = -1) {
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr)) return Fail("CoInitialize failed");

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) return Fail("Failed to create device enumerator");

        if (deviceIndex < 0) {
            // Default device
            hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
            if (FAILED(hr)) return Fail("Failed to get default audio endpoint");
        }
        else {
            // Enumerate all devices
            IMMDeviceCollection* pCollection = nullptr;
            hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
            if (FAILED(hr)) return Fail("Failed to enumerate audio endpoints");

            UINT count;
            hr = pCollection->GetCount(&count);
            if (FAILED(hr)) return Fail("Failed to get device count");

            if ((UINT)deviceIndex >= count) {
                pCollection->Release();
                return Fail("Invalid device index");
            }

            hr = pCollection->Item(deviceIndex, &pDevice);
            pCollection->Release();
            if (FAILED(hr)) return Fail("Failed to get selected audio device");
        }

        hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
        if (FAILED(hr)) return Fail("Failed to activate audio client");

        hr = pAudioClient->GetMixFormat(&pwfx);
        if (FAILED(hr)) return Fail("Failed to get mix format");

        REFERENCE_TIME bufferDuration = 10000000; // 1s

        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_LOOPBACK,
            bufferDuration,
            0,
            pwfx,
            NULL);
        if (FAILED(hr)) return Fail("AudioClient initialization failed");

        hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
        if (FAILED(hr)) return Fail("Failed to get capture client");

        initialized = true;
        return true;
    }

    void ListDevices() {
        HRESULT hr = CoInitialize(nullptr);
        if (FAILED(hr)) {
            printf("CoInitialize failed\n");
            return;
        }

        IMMDeviceEnumerator* pEnum = nullptr;
        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator), (void**)&pEnum);
        if (FAILED(hr)) {
            printf("Failed to create device enumerator\n");
            return;
        }

        IMMDeviceCollection* pCollection = nullptr;
        hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
        if (FAILED(hr)) {
            printf("Failed to enumerate audio endpoints\n");
            pEnum->Release();
            return;
        }

        UINT count = 0;
        hr = pCollection->GetCount(&count);
        if (FAILED(hr)) {
            printf("Failed to get device count\n");
            pCollection->Release();
            pEnum->Release();
            return;
        }

        for (UINT i = 0; i < count; ++i) {
            IMMDevice* pDevice = nullptr;
            hr = pCollection->Item(i, &pDevice);
            if (SUCCEEDED(hr)) {
                IPropertyStore* pProps = nullptr;
                hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
                if (SUCCEEDED(hr)) {
                    PROPVARIANT varName;
                    PropVariantInit(&varName);

                    hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
                    if (SUCCEEDED(hr)) {
                        printf("%u - %S\n", i, varName.pwszVal);
                        PropVariantClear(&varName);
                    }

                    pProps->Release();
                }
                pDevice->Release();
            }
        }

        pCollection->Release();
        pEnum->Release();
        CoUninitialize();
    }


    bool Start() {
        if (!initialized) return false;
        HRESULT hr = pAudioClient->Start();
        if (FAILED(hr)) return Fail("AudioClient failed to start");
        return true;
    }

    void Stop() {
        if (pAudioClient) pAudioClient->Stop();
    }

    WAVEFORMATEX* GetFormat() const {
        return pwfx;
    }

private:
    void Cleanup() {
        if (pwfx) CoTaskMemFree(pwfx);
        if (pCaptureClient) pCaptureClient->Release();
        if (pAudioClient) pAudioClient->Release();
        if (pDevice) pDevice->Release();
        if (pEnumerator) pEnumerator->Release();
        CoUninitialize();
    }

    bool Fail(const char* msg) {
        std::cerr << "[WASAPI] Error: " << msg << std::endl;
        return false;
    }
};