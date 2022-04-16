# IP_bridge
本文主要介绍下实现VPN隧道的思路， 如果只是想搭建VPN，建议参考open vpn等成熟的解决方案

之前一直是基于SSH 搭建隧道,主要使用brctl/ifconfig/ssh 等linux下成熟的工具搭建, 配置起来稍显复杂，因此萌生了
直接在windows下实现一个简单的隧道功能的想法

大体的原理就是在一个网卡上面抓包，然后通过TCP发送到远端，最后在远端的网卡上重新发送出网络包即可

https://www.winpcap.org/

主要的实现如下代码

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
        if (res == 0)    /* Note: res=0 means "read timeout elapsed"*/
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
