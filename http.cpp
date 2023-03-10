#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <WinSock2.h>
#pragma comment(lib, "WS2_32.lib")

#define PRINT(str) printf("[%s-%d]%s\n", __func__, __LINE__, str);

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

	// ???ö˿ڿɸ???
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

	while (i < size - 1 && c != '\n')
	{
		int n = recv(sock, &c, 1, 0);
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if (n > 0 && c == '\n')
				{
					recv(sock, &c, 1, 0);
				}
				else
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

void unimplement(int client) {
}

const char* getHeadType(const char* fileName)
{
	const char* ret = "text/html";
	const char* p = strrchr(fileName, '.');
	if (!p) {
		return ret;
	}
	p++;
	if (!strcmp(p, "css")) {
		return "text/css";
	}
	else if (!strcmp(p, "jpg")) {
		return "text/jpeg";
	}
	else if (!strcmp(p, "png")) {
		return "text/png";
	}
	else if (!strcmp(p, "js")) {
		return "application/x-javascript";
	}
	return ret;
}

void cat(int client, FILE* resource) {
	char buff[4096];
	int count = 0;

	while (1)
	{
		memset(buff, 0, 4096);
		int result = fread(buff, sizeof(char), sizeof(buff), resource);
		if (result <= 0) {
			break;
		}
		send(client, buff, result, 0);
		count += result;
		PRINT(buff);
	}
}

void not_found(int client) {
	char buff[1024];
	strcpy(buff, "HTTP/1.0 404 NOT FOUND\r\n");
	send(client, buff, strlen(buff), 0);
	PRINT(buff);

	strcpy(buff, "Server: Xing/0.1\r\n");
	send(client, buff, strlen(buff), 0);
	PRINT(buff);

	strcpy(buff, "Content-type:text/html\n");
	send(client, buff, strlen(buff), 0);
	PRINT(buff);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);

	FILE* resource = fopen("htdocs/404.html", "rb");
	cat(client, resource);
}

void headers(int client, const char* type) {
	char buff[1024];
	strcpy(buff, "HTTP/1.0 200 OK\r\n");
	send(client, buff, strlen(buff), 0);
	PRINT(buff);

	strcpy(buff, "Server: Xing/0.1\r\n");
	send(client, buff, strlen(buff), 0);
	PRINT(buff);

	char buf[1024];
	sprintf(buf, "Content-type: %s\r\n", type);
	send(client, buff, strlen(buff), 0);
	PRINT(buff);

	strcpy(buff, "\r\n");
	send(client, buff, strlen(buff), 0);
	PRINT(buff);
}

void server_file(int client, const char* fileName) {
	int num_chars = 1;
	char buff[1024];
	while (num_chars > 0 && strcmp(buff, "\n")) {
		num_chars = get_line(client, buff, sizeof(buff));
		PRINT(buff);
	}

	FILE* resource = NULL;
	if (strcmp(fileName, "htdocs/index.html") == 0) {
		resource = fopen(fileName, "r");
	}
	else {
		resource = fopen(fileName, "rb");
	}

	if (resource == NULL) {
		not_found(client);
	}
	else {
		headers(client, getHeadType(fileName));
		cat(client, resource);
		PRINT("resource sended");
	}
	fclose(resource);
}

DWORD WINAPI accept_request(LPVOID arg) {
	char buff[1024];
	int client = (SOCKET)arg;

	int num_chars = get_line(client, buff, sizeof(buff));

	char method[255];
	int j = 0, i = 0;
	while (!isspace(buff[j]) && i < sizeof(method) - 1) {
		method[i++] = buff[j++];
	}
	method[i] = 0;

	if (stricmp(method, "GET") && stricmp(method, "POST")) {
		unimplement(client);
		return 0;
	}

	while (isspace(buff[j]) && j < sizeof(buff)) {
		j++;
	}

	char url[255];
	i = 0;
	while (!isspace(buff[j]) && i < sizeof(url) - 1 && j < sizeof(buff)) {
		url[i++] = buff[j++];
	}
	url[i] = 0;

	char path[512] = "";
	sprintf(path, "htdocs%s", url);
	if (path[strlen(path) - 1] == '/') {
		strcat(path, "index.html");
	}
	PRINT(path);

	struct stat status;
	if (stat(path, &status) == -1) {
		while (num_chars > 0 && strcmp(buff, "\n")) {
			num_chars = get_line(client, buff, sizeof(buff));
		}
		not_found(client);
	}
	else {
		if ((status.st_mode & S_IFMT) == S_IFDIR) {
			strcat(path, "/index.html");
		}
		server_file(client, path);
	}
	closesocket(client);
	return 0;
}

int main(void) {
	unsigned short port = 2333;
	int server_sock = start_up(&port);
	printf("HTTP INITED. port: %d\n", port);

	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);
	while (1) {
		// ????ʽ?ȴ?
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