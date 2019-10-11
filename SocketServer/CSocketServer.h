#pragma once

#include <iostream>
#include <cassert>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment (lib, "WS2_32.lib")

class CSocketServer
{
public:
	CSocketServer(int PortNumber) : m_PortNumber{ PortNumber } {}
	~CSocketServer() { CleanUp(); }

	bool Initialize()
	{
		WSADATA wsaData{};
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			std::cerr << KErrorHeaderFailed << "initialize Windows Socket Application.\n";
			return false;
		}

		ADDRINFO* AddressInfo{};
		ADDRINFO Hints{};
		Hints.ai_family = AF_INET;
		Hints.ai_socktype = SOCK_STREAM;
		Hints.ai_protocol = IPPROTO_TCP;
		Hints.ai_flags = AI_PASSIVE;
		
		char cstrPortNumber[256]{};
		_itoa_s(htons(m_PortNumber), cstrPortNumber, 10);
		
		if (getaddrinfo(nullptr, cstrPortNumber, &Hints, &AddressInfo) != 0)
		{
			std::cerr << KErrorHeaderFailed << "get address information.\n";
			CleanUp();
			return false;
		}

		char HostName[KHostNameLength]{};
		if (gethostname(HostName, KHostNameLength) == SOCKET_ERROR)
		{
			std::cerr << KErrorHeaderFailed << "get host name.\n";
			freeaddrinfo(AddressInfo);
			CleanUp();
			return false;
		}
		std::clog << "Host name: " << HostName << '\n';

		m_SocketListen = socket(AddressInfo->ai_family, AddressInfo->ai_socktype, AddressInfo->ai_protocol);
		if (m_SocketListen == INVALID_SOCKET)
		{
			std::cerr << KErrorHeaderFailed << "create listening socket.\n";
			freeaddrinfo(AddressInfo);
			CleanUp();
			return false;
		}

		if (bind(m_SocketListen, AddressInfo->ai_addr, (int)AddressInfo->ai_addrlen) == SOCKET_ERROR)
		{
			std::cerr << KErrorHeaderFailed << "bind the socket to the address.\n";
			freeaddrinfo(AddressInfo);
			CleanUp();
			return false;
		}
		
		freeaddrinfo(AddressInfo); // bind() 성공 이후엔 필요 없다!
		
		getaddrinfo(HostName, cstrPortNumber, &Hints, &AddressInfo);
		if (AddressInfo->ai_family == AF_INET)
		{
			SOCKADDR_IN* SIN{ (sockaddr_in*)AddressInfo->ai_addr };
			char temp[4]{};

			_itoa_s(SIN->sin_addr.S_un.S_un_b.s_b1, temp, 10);
			strcat_s(m_HostIPAddress, temp);
			strcat_s(m_HostIPAddress, ".");

			_itoa_s(SIN->sin_addr.S_un.S_un_b.s_b2, temp, 10);
			strcat_s(m_HostIPAddress, temp);
			strcat_s(m_HostIPAddress, ".");
			
			_itoa_s(SIN->sin_addr.S_un.S_un_b.s_b3, temp, 10);
			strcat_s(m_HostIPAddress, temp);
			strcat_s(m_HostIPAddress, ".");

			_itoa_s(SIN->sin_addr.S_un.S_un_b.s_b4, temp, 10);
			strcat_s(m_HostIPAddress, temp);
		}
		std::clog << "Host IP address [" << m_HostIPAddress << "] port [" << m_PortNumber << "]\n";
		
		freeaddrinfo(AddressInfo);

		std::clog << KLogHeaderSucceeded << "initialize.\n";

		return true;
	}

	void Listen()
	{
		if (listen(m_SocketListen, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cerr << KErrorHeaderFailed << "listen.\n";
			CleanUp();
		}

		// 원래 listen()과 accept()는 별도의 스레드에서 계속 반복되며 실행되어야 한다..!
		// 그러려면 select()나 WSAPoll() 써야함??
		sockaddr SocketAddress{};
		int SocketAddressLength{ static_cast<int>(sizeof(SOCKADDR)) };
		m_SocketClient = accept(m_SocketListen, &SocketAddress, &SocketAddressLength);
		if (m_SocketClient == INVALID_SOCKET)
		{
			std::cerr << KErrorHeaderFailed << "accept the client.\n";
			CleanUp();
		}
		char AddressCString[INET_ADDRSTRLEN]{};
		if (InetNtop(AF_INET, SocketAddress.sa_data, AddressCString, INET_ADDRSTRLEN) == nullptr)
		{
			std::cerr << KErrorHeaderFailed << "figure out the client address.\n";
		}
		else
		{
			std::clog << "Client address: " << AddressCString << '\n';
		}
		std::clog << KLogHeaderSucceeded << "listen.\n";
	}

	bool Receive()
	{
		memset(m_ReceivingBuffer, 0, KReceivingBufferLength);

		m_cbReceived = recv(m_SocketClient, m_ReceivingBuffer, KReceivingBufferLength, 0);
		if (m_cbReceived > 0)
		{
			std::clog << KLogHeaderSucceeded << "receive [" << m_cbReceived << " bytes]\n";
			std::clog << m_ReceivingBuffer << '\n';

			// iResult == received byte count
			// Echo the buffer back to the sender
			if (send(m_SocketClient, m_ReceivingBuffer, m_cbReceived, 0) == SOCKET_ERROR)
			{
				std::cerr << KErrorHeaderFailed << "send.\n";
				return false;
			}

			std::clog << " - successfully echoed.\n";
		}
		else if (m_cbReceived == 0)
		{
			std::clog << KErrorHeaderFailed << "Connection is closing.\n";
			return false;
		}
		else
		{
			std::cerr << KErrorHeaderFailed << "receive.\n";
			return false;
		}
		return true;
	}

	void ShutDownSending()
	{
		// Shutdown sending part of the connection
		if (shutdown(m_SocketClient, SD_SEND) == SOCKET_ERROR)
		{
			std::cerr << KErrorHeaderFailed << "shutdown. [Error code: " << WSAGetLastError() << "]\n";
		}

		std::clog << KLogHeaderSucceeded << "shutdown.\n";
		
		CleanUp();
	}

private:
	void CleanUp()
	{
		closesocket(m_SocketListen);
		closesocket(m_SocketClient);
		WSACleanup();

		std::clog << KLogHeaderSucceeded << "clean up.\n";
	}

private:
	static constexpr int KHostNameLength{ 256 };
	static constexpr int KReceivingBufferLength{ 512 };
	static constexpr char KErrorHeaderFailed[19]{ "[ERROR] Failed to " };
	static constexpr char KLogHeaderSucceeded[20]{ "[LOG] Succeeded to " };

private:
	char m_HostIPAddress[INET6_ADDRSTRLEN]{};
	int m_PortNumber{};
	SOCKET m_SocketListen{ INVALID_SOCKET };
	SOCKET m_SocketClient{ INVALID_SOCKET };
	char m_ReceivingBuffer[KReceivingBufferLength]{};
	int m_cbReceived{};
};