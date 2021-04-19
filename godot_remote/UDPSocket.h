/* UDPSocket.h */
#pragma once

// https://github.com/wolfpld/tracy/blob/8f48d6e5802f0ec63c86aa6a3383d020ad0b9d44/common/TracySocket.hpp
#include <stdint.h>
#include <atomic>

struct addrinfo;
struct sockaddr;

class UdpBroadcast {
public:
	UdpBroadcast();
	~UdpBroadcast();

	bool Open(const char *addr, uint16_t port);
	void Close();

	int Send(uint16_t port, const void *data, int len);

	UdpBroadcast(const UdpBroadcast &) = delete;
	UdpBroadcast(UdpBroadcast &&) = delete;
	UdpBroadcast &operator=(const UdpBroadcast &) = delete;
	UdpBroadcast &operator=(UdpBroadcast &&) = delete;

private:
	int m_sock;
	uint32_t m_addr;
};

class IpAddress {
public:
	IpAddress();
	~IpAddress();

	void Set(const struct sockaddr &addr);

	uint32_t GetNumber() const { return m_number; }
	const char *GetText() const { return m_text; }

	IpAddress(const IpAddress &) = delete;
	IpAddress(IpAddress &&) = delete;
	IpAddress &operator=(const IpAddress &) = delete;
	IpAddress &operator=(IpAddress &&) = delete;

private:
	uint32_t m_number;
	char m_text[17];
};

class UdpListen {
public:
	UdpListen();
	~UdpListen();

	bool Listen(uint16_t port);
	bool IsListening();
	void Close();

	const char *Read(uint64_t &len, IpAddress &addr, int timeout);

	UdpListen(const UdpListen &) = delete;
	UdpListen(UdpListen &&) = delete;
	UdpListen &operator=(const UdpListen &) = delete;
	UdpListen &operator=(UdpListen &&) = delete;

private:
	int m_sock;
};
