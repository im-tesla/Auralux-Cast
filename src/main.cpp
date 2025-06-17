#include "stream/srt.h"
#include "loopback/wasapi.h"
#include <thread>
#include <vector>
#include <algorithm>

void help() {
    std::cout << "Usage:\n"
        << "  auracast.exe <ip> <port> [device_index]\n"
        << "Options:\n"
        << "  -list               List available audio render devices\n"
        << "  -help               Show this help message\n"
        << "\nExamples:\n"
        << "  program.exe 127.0.0.1 1234        (use default audio device)\n"
        << "  program.exe 10.0.0.167 1234 1     (use device index 1)\n"
        << "  program.exe -list                 (list all audio devices)\n";
}

int main(int argc, char* argv[]) {
	c_Wasapi wasapi;
	c_SRT srt;

    if (argc < 2 || std::string(argv[1]) == "-help" || std::string(argv[1]) == "--help") {
        help();
        return 0;
    }

    if (std::string(argv[1]) == "-list") {
        wasapi.ListDevices();
        return 0;
    }

    if (argc < 3) {
        std::cerr << "Error: IP and port must be specified.\n";
        help();
        return 1;
    }

    std::string ip = argv[1];
    int port = 0;
    int deviceIndex = -1;

    try {
        port = std::stoi(argv[2]);
    }
    catch (...) {
        std::cerr << "Error: Invalid port number.\n";
        return 1;
    }

    if (argc >= 4) {
        try {
            deviceIndex = std::stoi(argv[3]);
        }
        catch (...) {
            std::cerr << "Error: Invalid device index.\n";
            return 1;
        }
    }

	std::cout << "Starting audio capture and transmission via SRT...\n";
    std::cout << "Target: " << ip << ":" << port << "\n";

    if (!wasapi.Init(deviceIndex)) {
        std::cerr << "WASAPI Initialization failed\n";
        return 1;
    }

    if (!wasapi.Start()) {
        std::cerr << "Failed to start WASAPI\n";
        return 1;
    }

    if (!srt.InitSender(ip, port)) {
        std::cerr << "SRT Initialization failed, couldn't connect\n";
        return 1;
    }

    WAVEFORMATEX* format = wasapi.GetFormat();
    int frameSize = format->nBlockAlign;
    const size_t maxChunkSize = 1316;  // MESSAGE mode SRT packet limit :(

    std::cout << "Started transmitting audio via SRT\n";
    std::cout << "Audio Format Details:\n";
    std::cout << "  Channels: " << format->nChannels << "\n";
    std::cout << "  Sample Rate: " << format->nSamplesPerSec << " Hz\n";
    std::cout << "  Bits per Sample: " << format->wBitsPerSample << "\n";
    std::cout << "  Block Align: " << format->nBlockAlign << " bytes\n";
    std::cout << "  Avg Bytes Per Sec: " << format->nAvgBytesPerSec << "\n";
    std::cout << "  Format Tag: " << format->wFormatTag << "\n";

    std::cout << "Starting raw PCM stream...\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        UINT32 packetLength = 0;
        HRESULT hr = wasapi.pCaptureClient->GetNextPacketSize(&packetLength);
        if (FAILED(hr)) {
            std::cerr << "GetNextPacketSize failed\n";
            break;
        }

        while (packetLength != 0) {
            BYTE* pData = nullptr;
            UINT32 numFrames = 0;
            DWORD flags = 0;

            hr = wasapi.pCaptureClient->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
            if (FAILED(hr)) {
                std::cerr << "GetBuffer failed\n";
                break;
            }

            size_t dataSize = numFrames * frameSize;
            uint8_t* sendPtr = nullptr;

            std::vector<uint8_t> silent;
            if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
                silent.resize(dataSize, 0);
                sendPtr = silent.data();
            }
            else {
                sendPtr = reinterpret_cast<uint8_t*>(pData);
            }

            // chunk and send
            size_t remaining = dataSize;
            while (remaining > 0) {
                size_t chunkSize = std::clamp(remaining, size_t(1), maxChunkSize);
                if (!srt.Send(sendPtr, chunkSize)) {
                    std::cerr << "SRT send failed\n";
                    break;
                }
                sendPtr += chunkSize;
                remaining -= chunkSize;
            }

            hr = wasapi.pCaptureClient->ReleaseBuffer(numFrames);
            if (FAILED(hr)) {
                std::cerr << "ReleaseBuffer failed\n";
                break;
            }

            hr = wasapi.pCaptureClient->GetNextPacketSize(&packetLength);
            if (FAILED(hr)) {
                std::cerr << "GetNextPacketSize failed\n";
                break;
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
            std::cout << "Escape key pressed, stopping transmission...\n";
            break;
        }
    }

    wasapi.Stop();
    return 0;
}