//
// UDPListener - Richard Burns Rally - UDP Telemetry data logger, UDP listener class.
//
// Copyright (c) 2025, MIKA-N. www.RallySimFans.hu. All rights reserved. 
// 
// License: Do whatever you want with this application and code. Free to copy, share, modify, re-publish the code in both open and closed source applications,
//          but DO NOT HOLD the author liable for any direct or indirect damages or loss caused by this application.
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#include "UDPListener.h"

#pragma comment(lib, "ws2_32.lib")


CUDPListener::CUDPListener(int port)
{
    m_listenPort = port;
    m_socket = INVALID_SOCKET;
}

CUDPListener::~CUDPListener()
{
    Close();
}

bool CUDPListener::Initialize()
{
	if (m_socket != INVALID_SOCKET)
	{
		std::cerr << "WARNING. Already initialized. Shutdown the old UDP listener before initializing a new UDP listener." << std::endl;
		return false;
	}

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET)
    {
        std::cerr << "ERROR. UDP socket creation failed with error code " << ::WSAGetLastError() << std::endl;
        return false;
    }

    sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_listenPort);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        std::cerr << "ERROR. UDP port " << m_listenPort << " bind failed with error code " << ::WSAGetLastError() << std::endl;
        Close();
        return false;
    }

    std::cout << "Listening incoming RBR UDP data packets in port " << m_listenPort << std::endl;
    return true;
}

void CUDPListener::Close()
{
    if (m_socket != INVALID_SOCKET)
    {
        ::closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
}

int CUDPListener::ReceiveDataPacket(BYTE* buffer, int bufferSize)
{
    sockaddr_in senderAddr {};
    int senderAddrSize = sizeof(senderAddr);

	if (buffer == nullptr || bufferSize <= 0)
	{
		return 0;
	}

    int bytesReceived = ::recvfrom(m_socket, reinterpret_cast<char*>(buffer), bufferSize, 0, (sockaddr*)&senderAddr, &senderAddrSize);
    if (bytesReceived == SOCKET_ERROR)
    {
        int wsaErrorCode = ::WSAGetLastError();
        if (wsaErrorCode == WSANOTINITIALISED)
        {
            // Hmmm.. WSASocket services not initialized. Maybe the app is closing? Let's close the socket because we cannot use it anymore to read data
            Close();
        }

		if (wsaErrorCode != WSAEMSGSIZE)
		{
			std::cerr << "ERROR. recvfrom call failed with error code " << wsaErrorCode << std::endl;
            return 0;
		}

        // Accept WSAEMSGSIZE "too large, received data trucated" error because in theory a new NGP plugin version could append new data to
        // the telemetry struct data packet.
        // In that case we should get a new RBR/NGP telemetryData header file and a data packet struct definition and not just hope
        // the new version is a backward compatible with an old RBR telemetry struct definition.

        bytesReceived = bufferSize;
    }

	for(IN_ADDR clientAddr : m_knownClients)
    {
		if (clientAddr.S_un.S_addr == senderAddr.sin_addr.S_un.S_addr)
		{
            // The client is already known. No need to dump out the client IP number on screen again
            return bytesReceived;
		}
    }

    // A new client sending data here. Add client to "known clients" list and dump out the client IP number on screen
    m_knownClients.push_back(senderAddr.sin_addr);

    char szSenderIP[INET_ADDRSTRLEN]{};
    ::inet_ntop(AF_INET, &senderAddr.sin_addr, szSenderIP, INET_ADDRSTRLEN);
    std::cout << std::endl << szSenderIP << ":" << ::ntohs(senderAddr.sin_port) << " sent a data packet with size of " << bytesReceived << " bytes" << std::endl << std::endl;

    return bytesReceived;
}
