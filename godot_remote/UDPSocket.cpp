/* UDPSocket.cpp */

#include "UDPSocket.h"

// https://github.com/wolfpld/tracy/blob/8f48d6e5802f0ec63c86aa6a3383d020ad0b9d44/common/TracySocket.cpp

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <new>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#endif
#define poll WSAPoll
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

using namespace GRUtils;

#ifdef _WIN32
struct __wsinit {
	__wsinit() {
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
			fprintf(stderr, "Cannot init winsock.\n");
			exit(1);
		}
	}
};

void InitWinSock() {
	static __wsinit init;
}
#endif

#ifdef _WIN32
typedef SOCKET socket_t;
#else
typedef int socket_t;
#endif

UdpBroadcast::UdpBroadcast() :
		m_sock(-1) {
#ifdef _WIN32
	InitWinSock();
#endif
}

UdpBroadcast::~UdpBroadcast() {
	if (m_sock != -1) Close();
}

bool UdpBroadcast::Open(const char *addr, uint16_t port) {
	assert(m_sock == -1);

	struct addrinfo hints;
	struct addrinfo *res, *ptr;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if (getaddrinfo(addr, str(port).ascii().get_data(), &hints, &res) != 0) return false;
	int sock = 0;
	for (ptr = res; ptr; ptr = ptr->ai_next) {
		if ((sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)) == -1) continue;
#if defined __APPLE__
		int val = 1;
		setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
#endif
#if defined _WIN32
		unsigned long broadcast = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, sizeof(broadcast)) == -1)
#else
		int broadcast = 1;
		if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
#endif
		{
#ifdef _WIN32
			closesocket(sock);
#else
			close(sock);
#endif
			continue;
		}
		break;
	}
	freeaddrinfo(res);
	if (!ptr) return false;

	m_sock = sock;
	inet_pton(AF_INET, addr, &m_addr);
	return true;
}

void UdpBroadcast::Close() {
	assert(m_sock != -1);
#ifdef _WIN32
	closesocket(m_sock);
#else
	close(m_sock);
#endif
	m_sock = -1;
}

int UdpBroadcast::Send(uint16_t port, const void *data, int len) {
	assert(m_sock != -1);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = m_addr;
	return sendto(m_sock, (const char *)data, len, MSG_NOSIGNAL, (sockaddr *)&addr, sizeof(addr));
}

IpAddress::IpAddress() :
		m_number(0) {
	*m_text = '\0';
}

IpAddress::~IpAddress() {
}

void IpAddress::Set(const struct sockaddr &addr) {
#if defined _WIN32 && (!defined NTDDI_WIN10 || NTDDI_VERSION < NTDDI_WIN10)
	struct sockaddr_in tmp;
	memcpy(&tmp, &addr, sizeof(tmp));
	auto ai = &tmp;
#else
	auto ai = (const struct sockaddr_in *)&addr;
#endif
	inet_ntop(AF_INET, &ai->sin_addr, m_text, 17);
	m_number = ai->sin_addr.s_addr;
}

UdpListen::UdpListen() :
		m_sock(-1) {
#ifdef _WIN32
	InitWinSock();
#endif
}

UdpListen::~UdpListen() {
	if (m_sock != -1) Close();
}

bool UdpListen::Listen(uint16_t port) {
	assert(m_sock == -1);

	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) return false;

#if defined __APPLE__
	int val = 1;
	setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
#endif
#if defined _WIN32
	unsigned long reuse = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse));
#else
	int reuse = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#endif
#if defined _WIN32
	unsigned long broadcast = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, sizeof(broadcast)) == -1)
#else
	int broadcast = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
#endif
	{
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		return false;
	}

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock, (sockaddr *)&addr, sizeof(addr)) == -1) {
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		return false;
	}

	m_sock = sock;
	return true;
}

void UdpListen::Close() {
	assert(m_sock != -1);
#ifdef _WIN32
	closesocket(m_sock);
#else
	close(m_sock);
#endif
	m_sock = -1;
}

const char *UdpListen::Read(size_t &len, IpAddress &addr, int timeout) {
	static char buf[2048];

	struct pollfd fd;
	fd.fd = (socket_t)m_sock;
	fd.events = POLLIN;
	if (poll(&fd, 1, timeout) <= 0) return nullptr;

	sockaddr sa;
	socklen_t salen = sizeof(struct sockaddr);
	len = (size_t)recvfrom(m_sock, buf, 2048, 0, &sa, &salen);
	addr.Set(sa);

	return buf;
}
