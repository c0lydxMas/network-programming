#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <string>

#define BUFF_SIZE 2048
#define DELIMITER "\r\n"
#define LOGIN "1"
#define POST "2"
#define LOGOUT "3"

#pragma comment(lib, "Ws2_32.lib")

using namespace std;

SOCKET client;
sockaddr_in serverAddr;
char buffIn[BUFF_SIZE], buff[BUFF_SIZE];
bool isLogin = false;


void menu();
void sendMessage(string& res, SOCKET& client);
string receiveMessage();
void login();
void post();
void logout();

int main(int argc, char* argv[])
{
	//Validate parameters
	if (argc != 3)
	{
		std::cout << "Sample Pattern: Client.exe 127.0.0.1 6000\n";
		return 0;
	}
	int serverPort = std::stoi(argv[2]);

	// Initiate Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
	{
		std::cout << "Winsock 2.2 is not supported\n";
		return 0;
	}

	//Construct socket
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET)
	{
		std::cout << "Error " << WSAGetLastError() << ": Cannot create client socket.\n";
		return 0;
	}

	std::cout << "Client Started!\n";

	//Specify server address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

	//Request to connect server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr)))
	{
		std::cout << "Error " << WSAGetLastError() << ": Cannot connect server.\n";
		return 0;
	}

	std::cout << "Connected server!\n";

	menu();

	//Close socket
	closesocket(client);

	//Terminate winsock
	WSACleanup();

	return 0;
}


/* The sendMessage function to send message to client
* @param1 string of message to send to client
* @param2 SOCKET struct of connecting socket
*/
void sendMessage(string& res, SOCKET& connectedSocket)
{
	int messageLen = res.size();
	int idx = 0, nLeft = messageLen;
	res.append(DELIMITER);
	while (nLeft > 0)
	{
		int ret = send(connectedSocket, &res.c_str()[idx], messageLen, 0);
		nLeft -= ret;
		idx += ret;
		if (ret == SOCKET_ERROR)
			cout << "Error " << WSAGetLastError() << ": Cannot send data.\n";
	}
}

// The receiveMessage function return received message from server
string receiveMessage()
{
	memset(buff, 0, BUFF_SIZE);
	int ret = recv(client, buff, BUFF_SIZE, 0);
	if (ret == SOCKET_ERROR)
		cout << "Error " << WSAGetLastError() << ": Cannot receive data.\n";
	else if (strlen(buff) > 0)
	{
		buff[ret] = 0;
		return string(buff);
	}
	return "";
}

//The login function handle login request
void login() {
	if (isLogin == 1)
	{
		cout << "You are already logged in, you need to log out before logging in as different user.\n";
		return;
	}
	else
	{
		cout << "Enter username: ";
		string s, res;
		cin >> s;
		res.append(LOGIN);
		res.append(s);
		sendMessage(res, client);
		string rec = receiveMessage();
		if (rec.substr(0, 3) == "200")
		{
			cout << "You have logged in as " << s << '\n';
			isLogin = 1;
		}
		else if (rec.substr(0, 3) == "401")
			cout << "This account has been locked\n";
		else if (rec.substr(0, 3) == "402")
			cout << "Username doesn't exist\n";
		else if (rec.substr(0, 3) == "403")
			cout << "Account is logged in somewhere else\n";
	}
}

//The post function handle post request
void post()
{
	if (isLogin == 0)
	{
		cout << "Please login before posting!\n";
		return;
	}
	else
	{
		string res = POST;
		char buffer[BUFF_SIZE - 5];
		cout << "Enter post: ";
		cin.ignore(INT_MAX, '\n');
		cin.getline(buffer, sizeof(buffer));
		res.append(buffer);
		sendMessage(res, client);
		string rec = receiveMessage();
		if (rec.substr(0, 3) == "201")
			cout << "Post successfully!\n";
		else
			cout << "Post unsuccessfully!\n";
	}
}

//The logout function handle logout request
void logout()
{
	if (isLogin == 0)
	{
		cout << "You have to login before logging out!\n";
		return;
	}
	else
	{
		string res = LOGOUT;
		sendMessage(res, client);
		string rec = receiveMessage();
		if (rec.substr(0, 3) == "202")
		{
			cout << "Log out successfully!\n";
			isLogin = 0;
		}
		else
			cout << "Can't log out!\n";
	}
}


//The menu function display a menu interface for user
void menu()
{
	cout << "*****Welcome to my application*****\n";
	cout << "**********Here is the MENU*********\n";
	while (1)
	{
		cout << "\n1. Log in\n";
		cout << "2. Log out\n";
		cout << "3. Publish\n";
		cout << "4. Exit\n";
		cout << "Please select a function: ";
		string z;
		cin >> z;
		if (z == "1")
			login();
		else if (z == "2")
			logout();
		else if (z == "3")
			post();
		else if (z == "4")
		{
			closesocket(client);
			WSACleanup();
			exit(1);
		}
		else
			cout << "Invalid input: please enter an option listed above!\n";
	}
	return;
}




