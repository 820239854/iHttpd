#include <stdio.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

void error_die(const char* str) {
	perror(str);
	exit(1);
}

int start_up(unsigned short* port) {
	WSADATA data;
	int result = WSAStartup(MAKEWORD(1, 1), &data);
	if (result) {
		error_die("WSAStartup Failed");
	}

	int server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (server_socket == -1) {
		error_die("socket init failed");
	}

	// 设置端口可复用
	int opt = 1;
	result = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	if (result == -1) {
		error_die("setsockopt failed");
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	int nameLen = sizeof(server_addr);
	server_addr.sin_port = htons(*port);
	server_addr.sin_addr.s_addr = INADDR_ANY;

	result = bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if (result < 0) {
		error_die("bind failed");
	}

	if (*port == 0) {
		result = getsockname(server_socket, (struct sockaddr*)&server_addr, &nameLen);
		if (result < 0) {
			error_die("getsockname failed");
		}
		*port = server_addr.sin_port;
	}

	if (listen(server_socket, 5) < 0) {
		error_die("listen");
	}
	return server_socket;
}

int main(void) {
	unsigned short port = 0;
	int server_sock = start_up(&port);
	printf("HTTP INITED. port: %d", port);
	return 0;
}