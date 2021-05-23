#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <string>
#include "resource.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <process.h>

#define WM_SOCKET WM_USER + 1
#define MAX_CLIENT 1024
#define BUFF_SIZE 2048
#define SERVER_PORT 6000
#define SERVER_ADDR "127.0.0.1"
#define LOGIN_SUCCESS "200"
#define POST_SUCCESS "201"
#define LOGOUT_SUCCESS "202"
#define LOCK "401"
#define NOT_FOUND "402"
#define LOGIN_ALREADY "403"
#define LOGIN "1"
#define POST "2"
#define LOGOUT "3"
#define DELIMITER "\r\n"

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996)

using namespace std;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
HWND				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	windowProc(HWND, UINT, WPARAM, LPARAM);

//SOCKET client[MAX_CLIENT];
SOCKET listenSock;
char clientIp[INET_ADDRSTRLEN];
int clientPort;

typedef struct session {
	SOCKET connSock;
	string username, clientIp;
	int clientPort;
	bool isLogin;
}session;

session client[MAX_CLIENT];

void sendMessage(SOCKET& connectedSocket, string& res);
void login(session& newSession, string& res, string payload);
void publish(string& res);
void logout(session& newSession, string& res);
void logFile(session& newSession, string buffData, string res);
string getCurrentTime();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HWND serverWindow;

	//Registering the Window Class
	MyRegisterClass(hInstance);

	//Create the window
	if ((serverWindow = InitInstance(hInstance, nCmdShow)) == NULL)
		return FALSE;

	//Initiate WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		MessageBox(serverWindow, L"Winsock 2.2 is not supported.", L"Error!", MB_OK);
		return 0;
	}

	//Construct socket	
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//requests Windows message-based notification of network events for listenSock
	WSAAsyncSelect(listenSock, serverWindow, WM_SOCKET, FD_ACCEPT | FD_CLOSE | FD_READ);

	//Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, SERVER_ADDR, &serverAddr.sin_addr);

	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)))
	{
		MessageBox(serverWindow, L"Cannot associate a local address with server socket.", L"Error!", MB_OK);
	}

	//Listen request from client
	if (listen(listenSock, MAX_CLIENT)) {
		MessageBox(serverWindow, L"Cannot place server socket in state LISTEN.", L"Error!", MB_OK);
		return 0;
	}

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = windowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SERVER));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = L"WindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
HWND InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;
	int i;
	for (i = 0; i < MAX_CLIENT; i++)
		client[i].connSock = 0;
	hWnd = CreateWindow(L"WindowClass", L"WSAAsyncSelect TCP Server", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return hWnd;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_SOCKET	- process the events on the sockets
//  WM_DESTROY	- post a quit message and return
//
//

LRESULT CALLBACK windowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	SOCKET connSock;
	sockaddr_in clientAddr;
	int ret, clientAddrLen = sizeof(clientAddr), i = 0;
	char rcvBuff[BUFF_SIZE], sendBuff[BUFF_SIZE];

	switch (message) {
	case WM_SOCKET:
	{
		if (WSAGETSELECTERROR(lParam)) {
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i].connSock == (SOCKET)wParam) {
					closesocket(client[i].connSock);
					client[i].isLogin = 0;
					client[i].username = "";
					client[i].clientIp = "";
					client[i].clientPort = 0;
					client[i].connSock = 0;
					continue;
				}
		}

		switch (WSAGETSELECTEVENT(lParam)) {
		case FD_ACCEPT:
		{
			connSock = accept((SOCKET)wParam, (sockaddr*)&clientAddr, &clientAddrLen);
			if (connSock == INVALID_SOCKET) {
				break;
			}
		
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i].connSock == 0) {
					client[i].connSock = connSock;
					client[i].isLogin = 0;
					inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
					clientPort = ntohs(clientAddr.sin_port);
					client[i].clientPort = clientPort;
					client[i].clientIp = string(clientIp);
					//requests Windows message-based notification of network events for listenSock
					WSAAsyncSelect(client[i].connSock, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);
					break;
				}
			if (i == MAX_CLIENT)
				MessageBox(hWnd, L"Too many clients!", L"Notice", MB_OK);
		}
		break;

		case FD_READ:
		{
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i].connSock == (SOCKET)wParam)
					break;

			memset(rcvBuff, 0, BUFF_SIZE);

			ret = recv(client[i].connSock, rcvBuff, BUFF_SIZE, 0);
			if (ret > 0) {
				rcvBuff[ret] = 0;
				string res, buffData, flag, payload;
				buffData = string(rcvBuff);
				auto endPos = buffData.find(DELIMITER);
				flag = buffData.substr(0, 1);
				payload = buffData.substr(1, endPos - 1);
				if (flag == LOGIN)
					login(client[i], res, payload);
				else if (flag == POST)
					publish(res);
				else if (flag == LOGOUT)
					logout(client[i], res);
				//log to file	
				logFile(client[i], buffData, res);
				// send message to client
				sendMessage(client[i].connSock, res);
			}
		}
		break;

		case FD_CLOSE:
		{
			client[i].isLogin = 0;
			for (i = 0; i < MAX_CLIENT; i++)
				if (client[i].connSock == (SOCKET)wParam) {
					closesocket(client[i].connSock);
					client[i].username = "";
					client[i].clientIp = "";
					client[i].clientPort = 0;
					client[i].connSock = 0;
					break;
				}
		}
		break;
		}
	}
	break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;

	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
		shutdown(listenSock, SD_BOTH);
		closesocket(listenSock);
		WSACleanup();
		return 0;
	}
	break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}


/* The sendMessage function to send message to client
* @param1 SOCKET struct of connecting socket
* @param2 string of message to send to client
*/
void sendMessage(SOCKET& connectedSocket, string& res)
{
	int messageLen = res.size();
	int idx = 0, nLeft = messageLen;
	res += DELIMITER;
	while (nLeft > 0)
	{
		int ret = send(connectedSocket, &res.c_str()[idx], messageLen, 0);
		nLeft -= ret;
		idx += ret;
		if (ret == SOCKET_ERROR)
			cout << "Error " << WSAGetLastError() << ": Cannot send data.\n";
	}
}


/* The login function to handle login request
* @param1 struct of session to check login infomation
* @param2 string of message to send to client
* @param3 string of payload from message received from client
*/
void login(session& newSession, string& res, string payload)
{
		//read file account
		ifstream accountfile;
		accountfile.open("account.txt");
		string lineData;
		if (accountfile.fail())
			cout << "Cannot read file\n";
		else
		{
			int found = 0;
			while (!accountfile.eof())
			{
				getline(accountfile, lineData);
				size_t pos = lineData.find(" ");
				string username = lineData.substr(0, pos);
				string status = lineData.substr(pos + 1);
				if (username == payload)
				{
					found = 1;
					if (status == "1") {
						res = LOCK;
						break;
					}
					else
					{
						res = LOGIN_SUCCESS;
						newSession.isLogin = 1;
						newSession.username = username;
						break;
					}
				}
			}
			if (!found)
				res = NOT_FOUND;
		}
		accountfile.close();
}

/* The publish function to handle post request
* @param1 string of message to send to client
*/
void publish(string& res)
{
	res = POST_SUCCESS;
}

/* The logut function to handle logout request
* @param1 struct of session to save login infomation
* @param2 string of message to send to client
*/
void logout(session& newSession, string& res)
{
	res = LOGOUT_SUCCESS;
	newSession.isLogin = 0;
}

/* The logFile function log message to log_20183773.txt
* @param1 struct of session to get client info
* @param2 string of data received from client
* @param3 string as message to log
*/
void logFile(session& newSession, string buffData, string res)
{
	fstream logFile;
	logFile.open("log_20183773.txt", ios::out | ios::app);
	string str = newSession.clientIp + ":" + to_string(newSession.clientPort);
	str += " " + getCurrentTime() + " ";
	string flag = buffData.substr(0, 1);

	if (flag == LOGIN)
		flag = "USER";
	else if (flag == POST)
		flag = "POST";
	else if (flag == LOGOUT)
		flag = "QUIT";

	str += "$ " + flag + " " + buffData.substr(1) + " $ " + res.substr(0, 3);
	cout << str << '\n';
	if (logFile.is_open())
	{
		logFile << str + "\n";
		logFile.close();
	}
	else cout << "Can't to open file\n";
}

// The getCurrentTime function return the formated current time as a string
string getCurrentTime()
{
	time_t rawtime;
	tm* timeinfo;
	char buffer[50];
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	strftime(buffer, 50, "[%d/%m/%Y %H:%M:%S]", timeinfo);
	return string(buffer);
}
