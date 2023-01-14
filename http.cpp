#include <stdio.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

#define PRINT(str) printf("[%s -- %d]%s", __func__, __LINE__, str);

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

int get_line(int sock, char* buff, int size)
{
	char c = 0;
	int i = 0;

	while (i<size-1 && c != '\n')
	{
		int n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1 , MSG_PEEK);
				if(n>0 && c == '\n')
				{
					recv(sock, &c , 1, 0);
				}else
				{
					c = '\n';
				}
			}
			buff[i++] = c;
		}
		else
		{
			c = '\n';
		}
	}
	buff[i] = 0;
	return i;
}

DWORD WINAPI accept_request(LPVOID arg) {
	char buff[1024];
	int client = (SOCKET)arg;

	int num_chars = get_line(client, buff, sizeof(buff));
	PRINT(buff);
	return 0;
}

int main(void) {
	unsigned short port = 2333;
	int server_sock = start_up(&port);
	printf("HTTP INITED. port: %d\n", port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	while (1) {
		// 阻塞式等待
		int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
		if (client_sock == -1) {
			error_die("client_sock");
		}
		DWORD threadId = 0;
		CreateThread(0, 0, accept_request, (void*)client_sock, 0, &threadId);
	}
	closesocket(server_sock);
	return 0;
}