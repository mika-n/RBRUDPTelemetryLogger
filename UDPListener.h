//
// UDPListener - Richard Burns Rally - UDP Telemetry data logger, UDP listener class.
//
// Copyright (c) 2025, MIKA-N. www.RallySimFans.hu. All rights reserved. 
// 
// License: Do whatever you want with this application and code. Free to copy, share, modify, re-publish the code in both open and closed source applications,
//          but DO NOT HOLD the author liable for any direct or indirect damages or loss caused by this application.
//

#pragma once

#include <winsock2.h>
#include <vector>

class CUDPListener
{
private:
    std::vector<IN_ADDR> m_knownClients;
    int m_listenPort;
    SOCKET m_socket;

public:
    CUDPListener(int port);
    ~CUDPListener();

    bool Initialize();
    void Close();

    int ReceiveDataPacket(BYTE *buffer, int bufferSize);
    inline bool IsValidSocket() const { return m_socket != INVALID_SOCKET; }
};
