#pragma comment(lib, "ws2_32.lib")
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <time.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <iostream>

#define BUF_SIZE 100

using namespace std;

const int SEND_CHATDATA_USERNICKNAME = 1;
const int SEND_CHATDATA_USERMESSAGE = 2;

typedef struct _TimeStorage{
	int Year;
	int Month;
	int Date;
	int Hour;
	int Minute;
	int Second;
}TIME;

typedef struct _ChatDataStorage{
	int UserToken = -1;
	char Message[100];
	TIME SentTime;
}CHATING_CONTENT;


int chatStorageIdx = 0;
CHATING_CONTENT chatStorage[10000];

int nowToken = 10;

char UserTokenToNickname[100][100];

void CompressSockets(SOCKET hSockArr[], int idx, int total);
void CompressEvents(WSAEVENT hEventArr[], int idx, int total);
void ErrorHandling(char* message);

int main(int argc, char* argv[]) {
	WSADATA wsaData;
	SOCKET hServSock, hClntSock;
	SOCKADDR_IN servAdr, clntAdr;

	SOCKET hSockArr[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT hEventArr[WSA_MAXIMUM_WAIT_EVENTS];
	WSAEVENT newEvent;
	WSANETWORKEVENTS netEvents;
	
	int numOfClntSock = 0;
	int strLen;
	int posInfo, startIdx;
	int clntAdrLen;
	char msg[BUF_SIZE];
	
	if (argc != 2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		ErrorHandling((char*)"WSAStartUp() Error!");
	}

	hServSock = socket(PF_INET, SOCK_STREAM, 0);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR) {
		ErrorHandling((char*)"bind() Error!");
	}

	if (listen(hServSock, 5) == SOCKET_ERROR) {
		ErrorHandling((char*)"listen() Error!");
	}

	newEvent = WSACreateEvent();
	if (WSAEventSelect(hServSock, newEvent, FD_ACCEPT) == SOCKET_ERROR) {
		ErrorHandling((char*)"WSAEventSelect() Error!");
	}

	hSockArr[numOfClntSock] = hServSock;
	hEventArr[numOfClntSock] = newEvent;
	numOfClntSock++;

	while (1) {
		posInfo = WSAWaitForMultipleEvents(
			numOfClntSock, hEventArr, FALSE, WSA_INFINITE, FALSE);
		startIdx = posInfo - WSA_WAIT_EVENT_0;

		for (int i = startIdx; i < numOfClntSock; i++) {
			int sigEventIdx = WSAWaitForMultipleEvents(1, &hEventArr[i], TRUE, 0, FALSE);
			if ((sigEventIdx == WSA_WAIT_FAILED || sigEventIdx == WSA_WAIT_TIMEOUT)) {
				continue;
			} else {
				sigEventIdx = i;
				WSAEnumNetworkEvents(
					hSockArr[sigEventIdx], hEventArr[sigEventIdx], &netEvents);
				if (netEvents.lNetworkEvents & FD_ACCEPT) {
					if (netEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
						puts("Accept Error!");
						break;
					}
					clntAdrLen = sizeof(clntAdr);
					hClntSock = accept(
						hSockArr[sigEventIdx], (SOCKADDR*)&clntAdr, &clntAdrLen);
					newEvent = WSACreateEvent();
					WSAEventSelect(hClntSock, newEvent, FD_READ | FD_CLOSE);

					hEventArr[numOfClntSock] = newEvent;
					hSockArr[numOfClntSock] = hClntSock;

					char arr[10];
					memset(arr, 0, sizeof(arr));
					arr[0] = (char)nowToken;
					send(hSockArr[numOfClntSock], arr, sizeof((char*)nowToken), 0);
					nowToken++;
					numOfClntSock++;
					puts("connected new client...");
				}

				if (netEvents.lNetworkEvents & FD_READ) {
					if (netEvents.iErrorCode[FD_READ_BIT] != 0) {
						puts("Read Error");
						break;
					}

					strLen = recv(hSockArr[sigEventIdx], msg, sizeof(msg), 0);

					int typeHeader = (int)msg[0];
					int UserToken = (int)msg[1];
					char mainData[100];
					strcpy_s(mainData, 98, msg + 2);
					switch (typeHeader) {
					case 1: 
						strcpy_s(chatStorage[chatStorageIdx].Message, 100, mainData);
						chatStorage[chatStorageIdx].UserToken = UserToken;

						char temp[100];
						memset(temp, 0, sizeof(temp));
						temp[0] = (char)SEND_CHATDATA_USERNICKNAME;
						strcpy_s(temp + 1, 99, UserTokenToNickname[UserToken]);
						for (int i = 1; i <= numOfClntSock; i++) {
							send(hSockArr[i], temp, sizeof(temp), 0);
						}

						memset(temp, 0, sizeof(temp));
						temp[0] = (char)SEND_CHATDATA_USERMESSAGE;
						strcpy_s(temp + 1, 99, chatStorage[chatStorageIdx].Message);
						for (int i = 1; i <= numOfClntSock; i++) {
							send(hSockArr[i], temp, sizeof(temp), 0);
						}

						chatStorageIdx++;
						break;
					case 2:
						cout << "Set New NickName: " << UserToken << " to " << mainData << endl;
						strcpy_s(UserTokenToNickname[UserToken], 100, mainData);
						break;
					default:
						break;
					}
				}

				if (netEvents.lNetworkEvents & FD_CLOSE) {
					if (netEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
						puts("Close Error");
						break;
					}
					WSACloseEvent(hEventArr[sigEventIdx]);
					closesocket(hSockArr[sigEventIdx]);

					numOfClntSock--;
					CompressSockets(hSockArr, sigEventIdx, numOfClntSock);
					CompressEvents(hEventArr, sigEventIdx, numOfClntSock);
				}
			}
		}
	}
	WSACleanup();
	return 0;
}

void CompressSockets(SOCKET hSockArr[], int idx, int total) {
	for (int i = idx; i < total; i++) {
		hSockArr[i] = hSockArr[i + 1];
	}
}

void CompressEvents(WSAEVENT hEventArr[], int idx, int total) {
	for (int i = idx; i < total; i++) {
		hEventArr[i] = hEventArr[i + 1];
	}
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}