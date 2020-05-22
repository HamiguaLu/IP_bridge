
#include "UserBridge.h"

//Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")


/* Thread handlers. Global because we wait on the threads from the CTRL+C handler */
HANDLE captureThread;
HANDLE recvThread;

int g_adapterIndex = 0;
HWND g_dlgHandler = 0;
int g_serverPort = DEFAULT_PORT;
int g_speedTestMode = 0;
int g_serverMode = 0;
UINT64 g_txCount = 0;
UINT64 g_rxCount = 0;
UINT64 g_total_seconds = 0;
int g_capture = 0;

void logMessage(const char* fmt, ...)
{
#if 1
#pragma warning (disable : 4996)
	va_list argptr;
	char msg[1024];
	va_start(argptr, fmt);
	vsprintf_s(msg, 1024, fmt, argptr);
	va_end(argptr);

	char* logPath = "Bridge.log";

	FILE* pFile = fopen(logPath, "a");
	//logPath.ReleaseBuffer();
	fprintf(pFile, "%s\n", msg);
	fclose(pFile);
#endif
}

void updateMessage(const char* format, ...)
{
	va_list argptr;
	char msg[1024];
	va_start(argptr, format);
	vsprintf_s(msg, 1024, format, argptr);
	va_end(argptr);

	SendMessage(g_dlgHandler, WM_BRIDGE_MSG, (WPARAM)msg, 0);
}


/* This global variable tells the forwarder threads they must terminate */
int g_stopFlag = 0;

void stopBridge()
{
	g_stopFlag = 1;
}


/*******************************************************************/
u_int netmask;
pcap_t *getAdapter()
{
	pcap_if_t *alldevs;
	pcap_if_t *d;
	int i = 0;
	pcap_t *adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];


	/*
	* Retrieve the device list
	*/

	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1)
	{
		updateMessage("Error in pcap_findalldevs: %s", errbuf);
		exit(1);
	}


	/*
	* Open the specified couple of adapters
	*/

	/* Jump to the first selected adapter */
	for (d = alldevs, i = 0; i < g_adapterIndex; d = d->next, i++);

	/*
	* Open the first adapter.
	* *NOTICE* the flags we are using, they are important for the behavior of the prgram:
	*	- PCAP_OPENFLAG_PROMISCUOUS: tells the adapter to go in promiscuous mode.
	*    This means that we are capturing all the traffic, not only the one to or from
	*    this machine.
	*	- PCAP_OPENFLAG_NOCAPTURE_LOCAL: prevents the adapter from capturing again the packets
	*	  transmitted by itself. This avoids annoying loops.
	*	- PCAP_OPENFLAG_MAX_RESPONSIVENESS: configures the adapter to provide minimum latency,
	*	  at the cost of higher CPU usage.
	*/
	if ((adhandle = pcap_open(d->name,						// name of the device
		65536,							// portion of the packet to capture. 
										// 65536 grants that the whole packet will be captured on every link layer.
		PCAP_OPENFLAG_PROMISCUOUS |	// flags. We specify that we don't want to capture loopback packets, and that the driver should deliver us the packets as fast as possible
		PCAP_OPENFLAG_NOCAPTURE_LOCAL |
		PCAP_OPENFLAG_MAX_RESPONSIVENESS,
		500,							// read timeout
		NULL,							// remote authentication
		errbuf							// error buffer
	)) == NULL)
	{
		updateMessage("Unable to open the adapter. %s is not supported by WinPcap", d->description);
		/* Free the device list */
		pcap_freealldevs(alldevs);
		return NULL;
	}

	if (d->addresses != NULL)
	{
		/* Retrieve the mask of the first address of the interface */
		netmask = ((struct sockaddr_in *)(d->addresses->netmask))->sin_addr.S_un.S_addr;
	}
	else
	{
		/* If the interface is without addresses we suppose to be in a C class network */
		netmask = 0xffffff;
	}

	return adhandle;
}


void setSendBufSize(SOCKET s)
{
	int sendbuff;
	socklen_t optlen = sizeof(sendbuff);

	// Set buffer size
	sendbuff = TCP_SEND_BUFLEN;
	setsockopt(s, SOL_SOCKET, SO_SNDBUF, (const char*)&sendbuff, sizeof(sendbuff));
}


DWORD WINAPI ServerThread(LPVOID lpParameter)
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	SOCKADDR_IN serveraddr;

	struct addrinfo *result = NULL;

	//int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		updateMessage("WSAStartup failed with error: %d", iResult);
		return 1;
	}

	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET) {
		updateMessage("socket failed with error: %ld", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(g_serverPort);
	serveraddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	int opt = 1;
	setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (iResult == SOCKET_ERROR) {
		updateMessage("bind failed with error: %d", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		updateMessage("listen failed with error: %d", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	do
	{
		// Accept a client socket
		updateMessage("Waitting client connection...");
		ClientSocket = accept((unsigned int)ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET) {
			updateMessage("accept failed with error: %d", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		setSendBufSize(ClientSocket);

		updateMessage("Client connected,will start bridge now");
		start_bridge(ClientSocket);

		// cleanup
		closesocket(ClientSocket);
	} while (1);

	// No longer need server socket
	closesocket(ListenSocket);
	WSACleanup();

	return 0;

}


DWORD WINAPI clientThread(LPVOID lpParameter)
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	int iResult;

	SOCKADDR_IN *serverAddr = (SOCKADDR_IN*)lpParameter;
	serverAddr->sin_port = htons(g_serverPort);

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		updateMessage("WSAStartup failed with error: %d", iResult);
		return 1;
	}

	do
	{
		// Create a SOCKET for connecting to server
		ConnectSocket = socket(AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP);
		if (ConnectSocket == INVALID_SOCKET) {
			updateMessage("create socket failed with error: %ld", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		updateMessage("Connectting to server...");
		iResult = connect(ConnectSocket, (SOCKADDR*)serverAddr, sizeof(SOCKADDR));
		if (iResult == SOCKET_ERROR) {
			updateMessage("Can not connect to server error:%d", iResult);
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			//return -1;
			continue;
		}

		if (ConnectSocket == INVALID_SOCKET) {
			updateMessage("Unable to connect to server!");
			//WSACleanup();
			//return 1;
			continue;
		}

		setSendBufSize(ConnectSocket);

		updateMessage("Connected,will start bridge now");
		start_bridge(ConnectSocket);

		// cleanup
		closesocket(ConnectSocket);
	} while (1);

	WSACleanup();

	return 0;
}

int recvFixedLenghtData(SOCKET s, char *buf, int len)
{
	if (len < 1)
	{
		updateMessage("recv error len is:%d", len);
	}
	int pos = 0;
	while (len > 0)
	{
		int count = recv(s, buf + pos, len, 0);
		if (count <= 0)
		{
			updateMessage("recv error :%d,%d,%d", count, pos, len);
			return -1;
		}

		len -= count;
		pos += count;
	}

	return 0;

}

int speedTest(SOCKET remoteSocket) {
	char buf[200];
	int buf_len = 200;
	memset(buf, 0, 200);
	if (!g_serverMode)
	{
		//client only send
		while (1)
		{
			if (buf_len != send(remoteSocket, buf, buf_len, 0))
			{
				//failed to send package
				updateMessage("Network error:send failed,%d", buf_len);
				break;
			}

			g_txCount += buf_len;
		}

		return 0;
	}

	//this is server side, only recv
	do {
		int count = recv(remoteSocket, buf, buf_len, 0);
		if (count <= 0)
		{
			updateMessage("recv package failed  with error: %d", WSAGetLastError());
			break;
		}

		g_rxCount += count;
	} while (1);


	return 0;
}

int start_bridge(SOCKET remoteSocket)
{
	g_txCount = 0;
	g_rxCount = 0;
	g_total_seconds = 0;

	if (g_speedTestMode)
	{
		return speedTest(remoteSocket);
	}

	char *recvbuf = (char *)malloc(MAX_PACKAGE_SIZE);
	pcap_t *adhandle = getAdapter();

	//this will start another thread
	static in_out_adapters threadPara;
	threadPara.input_adapter = adhandle;
	threadPara.remoteSocket = remoteSocket;
	if ((captureThread = CreateThread(
		NULL,
		0,
		CaptureAndForwardThread,
		&threadPara,
		0,
		NULL)) == NULL)
	{
		updateMessage("error creating the forward thread");

		/* Close the adapters */
		pcap_close(adhandle);

		/* Free the device list */
		//pcap_freealldevs(alldevs);
		return -1;
	}

	// Receive the package from remote end and write to selected local adapter
	do {

		bpf_u_int32 pktLen = 0;

		if (0 != recvFixedLenghtData(remoteSocket, (char*)&pktLen, sizeof(pktLen)))
		{
			updateMessage("recv package length failed  with error: %d,len %d", WSAGetLastError(), pktLen);
			break;
		}

		pktLen = ntohl(pktLen);
		if (pktLen >= MAX_PACKAGE_SIZE)
		{
			updateMessage("recv failed with too large package: %d", pktLen);
			break;
		}

		if (0 != recvFixedLenghtData(remoteSocket, recvbuf, pktLen))
		{
			updateMessage("recv package failed with error: %d,len %d", WSAGetLastError(), pktLen);
			break;
		}

		//PostMessage(g_dlgHandler, WM_RX_COUNT, (WPARAM)(pktLen + sizeof(pktLen)), 0);
		g_rxCount += (pktLen + sizeof(bpf_u_int32));

		//send to eth card
		if (pcap_sendpacket(adhandle, recvbuf, pktLen) != 0)
		{
			updateMessage("Error sending %u bytes to interface:%s",
				pktLen,
				pcap_geterr(adhandle));

		}

	} while (1);

	/* Free the device list */
	//pcap_freealldevs(alldevs);

	free(recvbuf);

	WaitForSingleObject(captureThread, INFINITE);

	return 0;

}


/*******************************************************************
 * Forwarding thread.
 * Gets the packets from the input adapter and sends them to the output one.
 *******************************************************************/
DWORD WINAPI CaptureAndForwardThread(LPVOID lpParameter)
{
	struct pcap_pkthdr *header;
	const u_char *pkt_data;
	int res = 0;
	in_out_adapters* ad_couple = (in_out_adapters*)lpParameter;
	unsigned int n_fwd = 0;

	pcap_t *input_adapter = ad_couple->input_adapter;
	SOCKET remoteSocket = ad_couple->remoteSocket;

	/*
	 * Loop receiving packets from the first input adapter
	 */
	int timeOutCount = 0;
	while ((!g_stopFlag) && (res = pcap_next_ex(input_adapter, &header, &pkt_data)) >= 0)
	{
		if (res == 0)	/* Note: res=0 means "read timeout elapsed"*/
		{
			//updateMessage("pcap Read timeout elapsed");
			if (++timeOutCount > 200)
			{
				updateMessage("Cable disconnected?");
				timeOutCount = 0;
				break;
			}
			continue;
		}

		timeOutCount = 0;

		/*
		Send to other end over TCP
		*/
		bpf_u_int32 pktLen = htonl(header->caplen);
		if (sizeof(pktLen) != send(remoteSocket, (char *)&pktLen, sizeof(pktLen), 0))
		{
			//failed to send length
			updateMessage("Network error:send failed,%d", header->caplen);
			break;
		}

		if (header->caplen != send(remoteSocket, pkt_data, header->caplen, 0))
		{
			//failed to send package
			updateMessage("Network error:send failed,%d", header->caplen);
			break;
		}

		logMessage("captured pkt len %d", header->caplen);

		/*if (g_capture)
		{
			savePackage(pkt_data, header->caplen);
		}*/

		g_txCount += (header->caplen + sizeof(bpf_u_int32));
		//PostMessage(g_dlgHandler, WM_TX_COUNT, (WPARAM)(header->caplen + sizeof(pktLen)), 0);
	}

	/*
	 * We're out of the main loop. Check the reason.
	 */
	if (res < 0)
	{
		updateMessage("Error capturing the packets: %s", pcap_geterr(ad_couple->input_adapter));
		fflush(stdout);
	}
	else
	{
		updateMessage("End of bridging");
	}

	closesocket(remoteSocket);

	return 0;
}



void savePackage(const char *pkt, int len)
{
	FILE *file = fopen("e:\\traffice_dump_reference_only.pcapng", "a+b");
	if (file == NULL) {
		return;
	}

	fwrite(pkt, len, 1, file);
	fclose(file);
}

