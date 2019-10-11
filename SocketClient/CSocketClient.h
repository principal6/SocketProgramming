#pragma once

#include <iostream>
#include <cassert>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment (lib, "WS2_32.lib")
//#pragma comment (lib, "Mswsock.lib")
//#pragma comment (lib, "AdvApi32.lib")

class CSocketClient
{
public:
	CSocketClient(int PortNumber) : m_PortNumber{ PortNumber } {}
	~CSocketClient() { CleanUp(); }

	bool Initialize()
	{
		WSADATA wsaData{};
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << KErrorHeaderFailed << "initialize.\n";
			return false;
		}

		std::clog << KLogHeaderSucceeded << "initialize.\n";
		return true;
	}

	bool Connect(const char* ServerName)
	{
		ADDRINFO* AddressInfo{};
		ADDRINFO Hints{};
		Hints.ai_family = AF_UNSPEC; // IPv4, IPv6 둘 다 가능
		Hints.ai_socktype = SOCK_STREAM;
		Hints.ai_protocol = IPPROTO_TCP;

		char cstrPortNumber[255]{};
		_itoa_s(htons(m_PortNumber), cstrPortNumber, 10);
		if (getaddrinfo(ServerName, cstrPortNumber, &Hints, &AddressInfo) != 0)
		{
			std::cerr << KErrorHeaderFailed << "get address information.\n";
			return false;
		}
		std::clog << "Getting to server [" << ServerName << "] with port [" << m_PortNumber << "]\n";

		while (AddressInfo != nullptr)
		{
			m_SocketConnect = socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
			if (m_SocketConnect == INVALID_SOCKET)
			{
				std::cerr << KErrorHeaderFailed << "create socket.\n";
				return false;
			}

			if (connect(m_SocketConnect, AddressInfo->ai_addr, (int)AddressInfo->ai_addrlen) == SOCKET_ERROR)
			{
				std::cerr << KErrorHeaderFailed << "connect to " << AddressInfo->ai_addr << ".\n";
				AddressInfo = AddressInfo->ai_next;
			}
			else
			{
				std::clog << KLogHeaderSucceeded << "connect.\n";
				freeaddrinfo(AddressInfo);
				return true;
			}
		}

		std::cerr << KErrorHeaderFailed << "connect all the addresses.\nMake sure if the server is open.\n";
		freeaddrinfo(AddressInfo);
		closesocket(m_SocketConnect);
		m_SocketConnect = INVALID_SOCKET;
		return false;
	}

	bool Send(const char* Data, int DataByteCount)
	{
		m_cbSent = send(m_SocketConnect, Data, DataByteCount, 0);
		if (m_cbSent == SOCKET_ERROR)
		{
			std::cerr << KErrorHeaderFailed << "send.\n";
			return false;
		}
		std::clog << KLogHeaderSucceeded << "send.\n";
		return true;
	}

	void ShutDownSending()
	{
		if (shutdown(m_SocketConnect, SD_SEND) == SOCKET_ERROR)
		{
			std::cerr << KErrorHeaderFailed << "shutdown. [Error code: " << WSAGetLastError() << "]\n";
		}

		std::clog << KLogHeaderSucceeded << "shutdown.\n";
		CleanUp();
	}

	bool Receive()
	{
		memset(m_ReceivingBuffer, 0, KReceivingBufferLength);

		m_cbReceived = recv(m_SocketConnect, m_ReceivingBuffer, KReceivingBufferLength, 0);
		if (m_cbReceived > 0)
		{
			std::clog << KLogHeaderSucceeded << "receive [" << m_cbReceived << " bytes]\n";
			std::clog << m_ReceivingBuffer << '\n';
			return true;
		}
		else if (m_cbReceived == 0)
		{
			std::clog << KErrorHeaderFailed << "Connection is closed.\n";
			return false;
		}
		else
		{
			std::cerr << KErrorHeaderFailed << "receive.\n";
			return false;
		}
	}

private:
	void CleanUp()
	{
		closesocket(m_SocketConnect);
		WSACleanup();
	}

private:
	static constexpr int KReceivingBufferLength{ 512 };
	static constexpr char KErrorHeaderFailed[19]{ "[ERROR] Failed to " };
	static constexpr char KLogHeaderSucceeded[20]{ "[LOG] Succeeded to " };

private:
	int m_PortNumber{};
	SOCKET m_SocketConnect{ INVALID_SOCKET };
	char m_ReceivingBuffer[KReceivingBufferLength]{};
	int m_cbSent{};
	int m_cbReceived{};
};