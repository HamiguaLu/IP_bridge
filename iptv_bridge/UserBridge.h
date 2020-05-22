
#ifndef __USER_BRIDGE_HDR__
#define __USER_BRIDGE_HDR__

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcap.h"

#define TCP_SEND_BUFLEN (1000 * 1024) //1MB buffer for better performance over network
#define DEFAULT_PORT 32020

#define MAX_PACKAGE_SIZE   (TCP_SEND_BUFLEN * 5)

typedef struct _in_out_adapters
{
	pcap_t *input_adapter;
	SOCKET remoteSocket;
}in_out_adapters;

/* Prototypes */


//int readPackageThenSend(pcap_t *adhandle, SOCKET remoteSocket);




#ifdef __cplusplus
extern "C" {
#endif
	extern int g_adapterIndex;
	extern HWND g_dlgHandler;
	extern int g_serverPort;

	extern int g_speedTestMode;
	extern int g_serverMode;

	extern UINT64 g_txCount;
	extern UINT64 g_rxCount;
	extern UINT64 g_total_seconds;

	extern int g_capture;

	//extern int g_ethStatusChanged;
	
	int start_bridge(SOCKET remoteSocket);
	void stopBridge();
	void savePackage(const char *pkt, int len);

	DWORD WINAPI ServerThread(LPVOID lpParameter);
	DWORD WINAPI clientThread(LPVOID lpParameter);
	DWORD WINAPI dectectNetworkThread(LPVOID lparam);
	DWORD WINAPI CaptureAndForwardThread(LPVOID lpParameter);


#ifdef __cplusplus
};
#endif


#define WM_BRIDGE_MSG						(WM_USER + 1000)
#define WM_ETH_CABLE_STATUS_CHANGED			(WM_USER + 1003)


#define UI_UPDATE_INTEVAL  1


#endif