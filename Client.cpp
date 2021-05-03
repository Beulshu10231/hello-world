#pragma comment(lib,"ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <WinSock2.h>
#include <Windows.h>
#include <iostream>
#include <process.h>

using namespace std;

#define BUF_SIZE 1024

typedef struct _TimeStorage {
	int Year;
	int Month;
	int Date;
	int Hour;
	int Minute;
	int Second;
}TIME;

typedef struct _ChatDataStorage {
	char UserNickName[100];
	char Message[100];
	TIME SentTime;
}CHATING_CONTENT;


bool g_bLoop = true;
int chatStorageIdx = 0;
CHATING_CONTENT chatStorage[10000];

char UserInput[100];
int UserInputIdx = 0;

int myToken;

unsigned int __stdcall RecvThread(void* pArg);
void ScreenOutput();
void ErrorHandling(char* message);

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	SOCKET hSocket;
	char message[BUF_SIZE];
	int strLen;
	SOCKADDR_IN servAdr;

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling((char*)"WSAStartup() error!");
	}

	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET) {
		ErrorHandling((char*)"socket() error!");
	}

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(argv[1]);
	servAdr.sin_port = htons(atoi(argv[2]));

	if (connect(hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		ErrorHandling((char*)"connect() error!");
	}
	else
		puts("Connected...............");

	char arr[10];
	recv(hSocket, arr, sizeof(arr), 0);
	myToken = (int)arr[0];
	
	puts("사용할 닉네임을 입력:");
	char messageToSend[100];
	messageToSend[0] = (char)2;
	messageToSend[1] = (char)myToken;
	char tempScan[100];
	memset(tempScan, 0, sizeof(tempScan));
	scanf_s("%s", tempScan, 100);
	strcpy_s(messageToSend + 2, 98, tempScan);
	send(hSocket, messageToSend, sizeof(messageToSend), 0);

	system("cls");

	ScreenOutput();

	HANDLE hThread = (HANDLE)_beginthreadex(nullptr, 0, RecvThread, (void*)hSocket, 0, 0);

	while (1) {
		bool exitOn = false;

		bool isChanged = false;
		if (_kbhit()) {
			int tmp;
			switch (tmp = _getch()) {
			case 8:
				UserInputIdx--;
				break;
			case 13:
				if (UserInput[0] == '/') {
					if (strcmp(UserInput + 1, "exit")==0) {
						exitOn = true;
					}

				} else {
					messageToSend[0] = (char)1;
					messageToSend[1] = (char)myToken;
					strcpy_s(messageToSend + 2, 98, UserInput);
					UserInputIdx = 0;
					memset(UserInput, 0, sizeof(UserInput));
					send(hSocket, messageToSend, sizeof(messageToSend), 0);
				}
				break;
			default:
				UserInput[UserInputIdx++] = (char)tmp;
				break;
			}
			isChanged = true;
		}

		if (exitOn) {
			g_bLoop = false;
			break;
		}

		if (isChanged) {
			ScreenOutput();
		}
	}

	closesocket(hSocket);
	WSACleanup();
	return 0;
}

unsigned int __stdcall RecvThread(void* pArg) {
	SOCKET hSocket = (SOCKET)pArg;

	while (g_bLoop) {
		char msg[100] = {};
		recv(hSocket, msg, sizeof(msg), 0);

		int typeHeader = (int)msg[0];
		char mainData[100];
		strcpy_s(mainData, 99, msg + 1);

		switch (typeHeader) {
		case 1:
			strcpy_s(chatStorage[chatStorageIdx].UserNickName, 100, mainData);
			break;
		case 2:
			strcpy_s(chatStorage[chatStorageIdx++].Message, 100, mainData);
			break;
		}

		ScreenOutput();
	}
	return unsigned int();
}

void ScreenOutput() {
	system("cls");
	for (int i = 0; i < chatStorageIdx; i++) {
		cout << chatStorage[i].UserNickName << ": " << chatStorage[i].Message << endl;
	}

	cout << "------------------------";
	cout << endl;
	for (int i = 0; i < UserInputIdx; i++) {
		cout << UserInput[i];
	}
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}