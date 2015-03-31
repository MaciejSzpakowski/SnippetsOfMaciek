//WinSock2.h include must be before windows.h
#include <Ws2tcpip.h>
#include <WinSock2.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <process.h>
//need to link this thingy for winsock
#pragma comment(lib, "ws2_32.lib")

namespace winsock
{
	typedef char* STRING;
	HANDLE serverClientsMutex;
	HANDLE consoleMutex;
	#define strEqual(str1,str2,len) !memcmp(str1,str2,len)
	#define THREAD_FUNC unsigned(__stdcall*)(void*)
	enum SOCKET_TYPE { TYPE_SERVER = 1, TYPE_CLIENT = 2 };
	#define SUCCESS 1
	#define FAILURE 0
	#define enterMutex(handle) WaitForSingleObject(handle, INFINITE)
	#define exitMutex(handle) ReleaseMutex(handle)
	#define clearStdin() FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE))
	#define isKeyDown(key) GetAsyncKeyState(key) & 0x80000000
	#define isThreadRunning(handle) WaitForSingleObject( handle, 0) != WAIT_OBJECT_0

	//for now WSAGetLastError is the same as GetLastError but it might not be in the future
	//returns error code
	DWORD printLastSocketError(STRING msg)
	{
		DWORD err = WSAGetLastError();
		char str[300];
		SecureZeroMemory(str, 300);
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0,
			err, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
			str, 300, 0);
		DWORD written;
		enterMutex(consoleMutex);
		printf("Error %i. ",err);
		printf("%s\n", msg);
		WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), str, strchr(str, 0) - str, &written, 0);
		exitMutex(consoleMutex);
		return err;
	}

	struct SV_CLIENT
	{
		HANDLE receiveThreadHandle;
		SOCKET socket;
		sockaddr_in address;
	};

	struct NODE
	{
		SV_CLIENT* client;
		NODE* next;
	};	

	struct CLIENT
	{
		HANDLE receiveThreadHandle;
		SOCKET socket;
		SOCKET server;
		sockaddr_in serverAddress;
	};	

	struct SERVER
	{
		BOOL running;
		HANDLE acceptThreadHandle;
		sockaddr_in address;
		SOCKET socket;
		NODE* clients;
	};

	//clients are added to the end of the linked list
	//put this in mutex
	void addClient(SERVER* server, SV_CLIENT* newClient)
	{
		NODE* newNode = (NODE*)malloc(sizeof(NODE));
		newNode->next = NULL;
		newNode->client = newClient;
		if (server->clients == NULL)
			server->clients = newNode;
		else
		{
			NODE* back = server->clients;
			while (back->next != NULL)
				back = back->next;
			back->next = newNode;
		}
	}

	//put this in mutex
	void removeClient(SERVER* server, SV_CLIENT* client)
	{
		NODE* prev = NULL;
		NODE* toRemove = server->clients;
		//get node
		while (toRemove->client != client)
		{
			prev = toRemove;
			toRemove = toRemove->next;
		}
		//new head
		if (toRemove == server->clients)
			server->clients = server->clients->next;
		//relink
		else
			prev->next = toRemove->next;
		//shutdown and free
		if (shutdown(toRemove->client->socket, SD_BOTH) == SOCKET_ERROR)
			printLastSocketError("shutdown() in removeClient()");
		TerminateThread(client->receiveThreadHandle, 0);
		WaitForSingleObject(client->receiveThreadHandle, INFINITY);
		CloseHandle(client->receiveThreadHandle);
		free(toRemove->client);
		free(toRemove);
	}

	void getUInt(int* dst)
	{		
		printf("> ");
		char buffer[100];
		clearStdin();
		fgets(buffer, 100, stdin);
		if (isdigit(buffer[0]))
			sscanf(buffer, "%i", dst);
	}

	void getString(char* dst, int maxSize)
	{		
		printf("> ");
		clearStdin();
		fgets(dst,maxSize,stdin);
		//damn new line character
		for (int i = 0; i < maxSize;i++)
			if (dst[i] == '\n')
			{
				dst[i] = 0;
				break;
			}
	}

	void getIpFromClient(SV_CLIENT* client, STRING result, int bufSize)
	{
		inet_ntop(AF_INET, &(client->address.sin_addr), result, bufSize);
	}

	void receiveThreadServer(SV_CLIENT* client)
	{
		char buffer[100];
		memset(buffer, 0, sizeof(char) * 100);
		int index = 0;
		int currentLength = 0;
		while (TRUE)
		{
			char tempBuffer[100];
			int tempIndex = 0;
			enterMutex(consoleMutex);
			printf("Receiving...\n");
			exitMutex(consoleMutex);
			int len = recv(client->socket, tempBuffer, 100, NULL);
			enterMutex(consoleMutex);
			printf("Received %s \n", tempBuffer);
			exitMutex(consoleMutex);
			if (len == SOCKET_ERROR)
			{
				//check in server obj if this is still running
				printLastSocketError("recv() in receiveThreadServer()");
				return;
			}
			if (currentLength == 0)
				currentLength = tempBuffer[tempIndex++];
			for (int i = tempIndex; i < len; i++)
			{
				buffer[index++] = tempBuffer[i];
			}
			if (index == currentLength)
			{
				char ip[100];
				getIpFromClient(client, ip, 100);
				enterMutex(consoleMutex);
				printf("Message from %s - %s\n", ip, buffer);
				exitMutex(consoleMutex);
				memset(buffer, 0, sizeof(char) * 100);
				index = 0;
				currentLength = 0;
			}
		}
	}

	int receiveThreadClient(CLIENT* client)
	{
		char buffer[100];
		memset(buffer, 0, sizeof(char) * 100);
		int index = 0;
		int currentLength = 0;
		while (TRUE)
		{
			char tempBuffer[100];
			int tempIndex = 0;
			int len = recv(client->socket, tempBuffer, 100, NULL);
			if (len == SOCKET_ERROR)
			{
				//check in server obj if this is still running
				if(printLastSocketError("recv() in receiveThreadClient()") == 10054)
					return 1;
			}
			if (currentLength == 0)
				currentLength = tempBuffer[tempIndex++];
			for (int i = tempIndex; i < len; i++)
			{
				buffer[index++] = tempBuffer[i];
			}
			if (index == currentLength)
			{
				enterMutex(consoleMutex);
				printf("Message from server - %s\n", buffer);
				exitMutex(consoleMutex);
				memset(buffer, 0, sizeof(char) * 100);
				index = 0;
				currentLength = 0;
			}
		}
	}

	void acceptThread(SERVER* server)
	{
		while (server->running)
		{
			//server
			//accept connection
			//accept functions blocks further thread execution so it requires a separate thread			
			SV_CLIENT* client = (SV_CLIENT*)malloc(sizeof(SV_CLIENT));			
			int i1 = sizeof(sockaddr_in);
			client->socket = accept(server->socket, (sockaddr*)&(client->address), &i1);
			if (client->socket == INVALID_SOCKET)
			{
				printLastSocketError("accept() in acceptThread()");
				free(client);
				continue;
			}
			enterMutex(serverClientsMutex);
			client->receiveThreadHandle = (HANDLE)_beginthreadex(NULL, NULL, (THREAD_FUNC)receiveThreadServer, client, NULL, NULL);
			addClient(server, client);			
			char ip[100];
			getIpFromClient(client, ip, 100);
			exitMutex(serverClientsMutex);
			enterMutex(consoleMutex);
			printf("New client connected: %s\n", ip);
			exitMutex(consoleMutex);
		}
	}	

	void startAccepting(SERVER* server)
	{
		server->acceptThreadHandle = (HANDLE)_beginthreadex(NULL, NULL, (THREAD_FUNC)acceptThread, server, NULL, NULL);
	}

	int init(SERVER* server, CLIENT* client, WSADATA* socketData, SOCKET_TYPE* type)
	{
		//initialize winsock library
		if (WSAStartup(0x0002, socketData) != 0)
		{
			printLastSocketError("WSAStartup() in init()");
			return FAILURE;
		}
		int input = -1;
		//server or client
		printf("1. For server  2. For client\n");
		while (input != 1 && input != 2)
			getUInt(&input);
		*type = (SOCKET_TYPE)input;

		if (*type == TYPE_SERVER)
		{
			//port
			int port = 0;
			printf("Type port\n");
			while (port == 0)
				getUInt(&port);
			//create socket, AF_INET,SOCK_STREAM,NULL for tcp/ip v4, AF_INET6 for ip v6
			server->running = TRUE;
			server->socket = socket(AF_INET, SOCK_STREAM, NULL);
			if (server->socket == INVALID_SOCKET)
			{
				printLastSocketError("server's socket() in init()");
				return FAILURE;
			}
			//bind socket to address
			server->address.sin_port = htons(port); //port
			server->address.sin_addr.s_addr = htonl(INADDR_ANY); //what address accept
			server->address.sin_family = AF_INET;
			//address.sin_zero =  idk what that is
			if (bind(server->socket, (sockaddr*)&(server->address), sizeof(sockaddr)) == SOCKET_ERROR)
			{
				printLastSocketError("server's bind() in init()");
				return FAILURE;
			}
			//start listening
			if (listen(server->socket, 100) == SOCKET_ERROR) //second number: The maximum length of the queue of pending connections
			{
				printLastSocketError("server's listen() in init()");
				return FAILURE;
			}
			//start accepting
			startAccepting(server);
		}
		else if (*type == TYPE_CLIENT)
		{
			//ip
			char ip[100];
			printf("Type IP\n");
			getString(ip, 100);
			//create socket, AF_INET,SOCK_STREAM,NULL for tcp/ip v4, AF_INET6 for ip v6
			client->socket = socket(AF_INET, SOCK_STREAM, NULL);
			if (client->socket == INVALID_SOCKET)
			{
				printLastSocketError("client's socket() in init()");
				return FAILURE;
			}			
			client->serverAddress.sin_family = AF_INET;
			//this is how to create ip address from string
			inet_pton(AF_INET, ip, &client->serverAddress.sin_addr);
			//port
			int port = 0;
			printf("Type port\n");
			while (port == 0)
				getUInt(&port);
			client->serverAddress.sin_port = htons(port);
			//connect
			if (connect(client->socket, (sockaddr*)&(client->serverAddress), sizeof(sockaddr_in)) != 0)
			{
				printLastSocketError("client's connect() in init()");
				return FAILURE;
			}
			//start receiving thread
			client->receiveThreadHandle = (HANDLE)_beginthreadex(NULL, NULL, (THREAD_FUNC)receiveThreadClient, client, NULL, NULL);
			printf("Successfully connected\n");
		}
		return SUCCESS;
	}	

	int shutdownServer(SERVER* server)
	{
		//stop accepting
		server->running = FALSE;
		enterMutex(serverClientsMutex);
		TerminateThread(server->acceptThreadHandle, 0);
		exitMutex(serverClientsMutex);
		WaitForSingleObject(server->acceptThreadHandle, INFINITE);
		//close client socket, both send and receive
		while (server->clients != NULL)
			removeClient(server, server->clients->client);
		//close server socket
		closesocket(server->socket);
		//uninitialize winsock library
		if (WSACleanup() != 0)
		{
			printLastSocketError("WSACleanup() in shutdownServer()");
			return FAILURE;
		}
		return SUCCESS;
	}

	int shutdownClient(CLIENT* client)
	{
		//close server socket
		closesocket(client->socket);
		//uninitialize winsock library
		if (WSACleanup() != 0)
		{
			printLastSocketError("WSACleanup() in shutdownClient()");
			return FAILURE;
		}
		return SUCCESS;
	}

	void sendMessage(char* msg, SERVER* server, CLIENT* client, SOCKET_TYPE sType)
	{
		if (msg[0] == 0)
			return;
		if (sType == TYPE_CLIENT)
		{
			STRING smsg = msg - 1;
			int result = 0;
			int len = strlen(msg);
			//first byte is msg length
			smsg[0] = len;
			//send function isnt guaranteed to send everything
			//compare return value with msg len and resend if necessary
			while (result < len)
			{
				smsg += result;
				len = strlen(smsg);

				result = send(client->socket, smsg, len, NULL);
				if (result == SOCKET_ERROR)
				{
					printLastSocketError("client's send() in sendMessage()");
					return;
				}
			}
		}
		else if (sType == TYPE_SERVER)
		{
			NODE* node = server->clients;
			while (node != NULL)
			{
				STRING smsg = msg - 1;
				int result = 0;
				int len = strlen(msg);
				//first byte is msg length
				smsg[0] = len;
				//send function isnt guaranteed to send everything
				//compare return value with msg len and resend if necessary
				while (result < len)
				{
					smsg += result;
					len = strlen(smsg);

					result = send(node->client->socket, smsg, len, NULL);
					if (result == SOCKET_ERROR)
					{
						printLastSocketError("server's send() in sendMessage()");
						return;
					}
				}
				node = node->next;
			}
		}
	}

	//put this in mutex
	void checkReceivingThreads(SERVER* server, CLIENT* client, SOCKET_TYPE sType)
	{
		if (sType == TYPE_SERVER)
		{
			NODE* node = server->clients;
			while (node != NULL)
			{
				if (!isThreadRunning(node->client->receiveThreadHandle))
				{
					NODE* toRemove = node;
					node = node->next;
					removeClient(server, toRemove->client);
				}
				else
					node = node->next;
			}
		}
	}

	void userInterface(SERVER* server, CLIENT* client, SOCKET_TYPE sType)
	{
		HWND window = GetConsoleWindow();
		char buffer[100];
		while (strEqual(buffer, "exit", 4) == FALSE)
		{
			memset(buffer, 0, sizeof(char) * 100);
			enterMutex(consoleMutex);
			if (isKeyDown('C') && window == GetForegroundWindow())
				getString(buffer, 100);
			if (strEqual(buffer, "send ", 5))
				sendMessage(buffer + 5, server, client, sType);
			//enumerate all connected clients
			if (strEqual(buffer, "enum ", 4) && sType == TYPE_SERVER)
			{
				enterMutex(serverClientsMutex);
				NODE* node = server->clients;
				int total = 0;
				while (node != NULL)
				{
					char ip[100];
					getIpFromClient(node->client, ip, 100);
					printf("%s\n", ip);
					total++;
					node = node->next;
				}
				printf("Total: %i\n",total);
				exitMutex(serverClientsMutex);
			}
			exitMutex(consoleMutex);
			enterMutex(serverClientsMutex);
			checkReceivingThreads(server, client, sType);
			exitMutex(serverClientsMutex);
			Sleep(50);
		}
	}

	int socketMain()
	{
		serverClientsMutex = CreateMutex(NULL, FALSE, NULL);
		consoleMutex = CreateMutex(NULL, FALSE, NULL);
		SOCKET_TYPE sType;
		WSADATA socketData;
		SERVER server;
		SecureZeroMemory(&server, sizeof(server));
		CLIENT client;
		SecureZeroMemory(&client, sizeof(client));
		if (init(&server, &client, &socketData, &sType) == FAILURE)
			return 1;		
		
		userInterface(&server, &client, sType);

		if (sType == TYPE_SERVER)
			shutdownServer(&server);
		if (sType == TYPE_CLIENT)
			shutdownClient(&client);
		CloseHandle(serverClientsMutex);
		CloseHandle(consoleMutex);
		return 0;
	}

}
