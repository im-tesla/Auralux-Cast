#pragma once
#include <srt/srt.h>
#include <string>
#include <iostream>

class c_SRT {
public:
    c_SRT() : sock(SRT_INVALID_SOCK), isConnected(false) {}

    ~c_SRT() {
        Close();
    }

    bool InitSender(const std::string& host, int port, int latencyMs = 200) {
        if (srt_startup() != 0) {
            std::cerr << "[SRT] Failed to initialize\n";
            return false;
        }

        sockaddr_in sa;
        memset(&sa, 0, sizeof sa);
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        inet_pton(AF_INET, host.c_str(), &sa.sin_addr);

        sock = srt_create_socket();
        if (sock == SRT_INVALID_SOCK) {
            std::cerr << "[SRT] Failed to create socket\n";
            return false;
        }

        srt_setsockopt(sock, 0, SRTO_LATENCY, &latencyMs, sizeof(latencyMs));
        int streamMode = 0;
        srt_setsockopt(sock, 0, SRTO_TRANSTYPE, &streamMode, sizeof(streamMode));

        if (srt_connect(sock, (sockaddr*)&sa, sizeof sa) == SRT_ERROR) {
            std::cerr << "[SRT] Failed to connect: " << srt_getlasterror_str() << "\n";
            return false;
        }

        isConnected = true;
        std::cout << "[SRT] Connected to " << host << ":" << port << "\n";
        return true;
    }

    bool Send(const uint8_t* data, size_t size) {
        if (!isConnected) return false;

        int sent = srt_send(sock, (const char*)data, (int)size);
        if (sent == SRT_ERROR) {
            std::cerr << "[SRT] Send error: " << srt_getlasterror_str() << "\n";
            return false;
        }

        return true;
    }

    bool InitReceiver(int port) {
        if (srt_startup() != 0) {
            std::cerr << "[SRT] Failed to initialize\n";
            return false;
        }

        sockaddr_in sa;
        memset(&sa, 0, sizeof sa);
        sa.sin_family = AF_INET;
        sa.sin_port = htons(port);
        sa.sin_addr.s_addr = INADDR_ANY;

        sock = srt_create_socket();
        if (sock == SRT_INVALID_SOCK) return false;

        int streamMode = 1;
        srt_setsockopt(sock, 0, SRTO_TRANSTYPE, &streamMode, sizeof(streamMode));

        if (srt_bind(sock, (sockaddr*)&sa, sizeof sa) == SRT_ERROR) {
            std::cerr << "[SRT] Bind failed\n";
            return false;
        }

        if (srt_listen(sock, 1) == SRT_ERROR) {
            std::cerr << "[SRT] Listen failed\n";
            return false;
        }

        sockaddr_in their_addr;
        int addrlen = sizeof(their_addr);
        SRTSOCKET new_sock = srt_accept(sock, (sockaddr*)&their_addr, &addrlen);
        if (new_sock == SRT_INVALID_SOCK) {
            std::cerr << "[SRT] Accept failed\n";
            return false;
        }

        srt_close(sock);
        sock = new_sock;
        isConnected = true;
        std::cout << "[SRT] Receiver connected\n";
        return true;
    }

    int Receive(uint8_t* buffer, size_t maxSize) {
        if (!isConnected) return -1;
        return srt_recv(sock, (char*)buffer, (int)maxSize);
    }

    void Close() {
        if (sock != SRT_INVALID_SOCK) {
            srt_close(sock);
            sock = SRT_INVALID_SOCK;
        }
        if (isConnected) {
            srt_cleanup();
            isConnected = false;
        }
    }

private:
    SRTSOCKET sock;
    bool isConnected;
};