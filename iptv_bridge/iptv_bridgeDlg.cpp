
// iptv_bridgeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "iptv_bridge.h"
#include "iptv_bridgeDlg.h"
#include "afxdialogex.h"
#include "UserBridge.h"
#include "ini.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Ciptv_bridgeDlg dialog



Ciptv_bridgeDlg::Ciptv_bridgeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_IPTV_BRIDGE_DIALOG, pParent)
	, m_ServerPort(DEFAULT_PORT)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Ciptv_bridgeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_ADAPTER_LIST, m_adapterList);
	DDX_Control(pDX, IDC_NETADDRESS_REMOTE, m_remoteServerAddr);
	DDX_Control(pDX, IDC_STATIC_WORK_MDDE, m_serverWorkMode);
	DDX_Control(pDX, IDC_STATIC_TRANSFER_STATUS, m_transferStatus);
	DDX_Control(pDX, IDC_EDIT_LOG, m_logConsole);
	DDX_Text(pDX, IDC_EDIT_PORT, m_ServerPort);
	DDV_MinMaxInt(pDX, m_ServerPort, 10000, 60000);
}

BEGIN_MESSAGE_MAP(Ciptv_bridgeDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &Ciptv_bridgeDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &Ciptv_bridgeDlg::OnBnClickedCancel)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CHECK_CAPTURE, &Ciptv_bridgeDlg::OnBnClickedCheckCapture)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// Ciptv_bridgeDlg message handlers

BOOL Ciptv_bridgeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	pcap_if_t *alldevs;
	pcap_if_t *d;
	//int inum;
	int i = 0;
	//pcap_t *adhandle;
	char errbuf[PCAP_ERRBUF_SIZE];

	/* Retrieve the device list */
	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		fprintf(stderr, "Error in pcap_findalldevs: %s\n", errbuf);
		exit(1);
	}

	int count = 0;
	/* Print the list */
	for (d = alldevs; d; d = d->next)
	{
		CString sAdapter;
		sAdapter.Format(_T("%d  "), ++i);
		if (d->description)
			sAdapter += d->description;


		/*add to the list*/
		m_adapterList.AddString(sAdapter);
		++count;
	}

	pcap_freealldevs(alldevs);
	if (count <= 1)
	{
		AfxMessageBox(_T("Must have at least 2 eth adapters to create bridge"), MB_OK | MB_ICONSTOP);
		this->OnCancel();
	}

	m_remoteServerAddr.SetAllowType(NET_STRING_IPV4_ADDRESS);

	//m_remoteServerAddr.SetWindowTextW(_T("10.17.83.190"));

	addLog(_T("!!!Please allow the tool through your firewall!!!"));
	addLog(_T("!!!Please allow the tool through your firewall!!!"));
	addLog(_T("!!!Please allow the tool through your firewall!!!"));

	SetDlgItemTextW(IDC_STATIC_TRANSFER_STATUS, _T(""));
	SetDlgItemTextW(IDC_STATIC_TX, _T(""));
	//SetDlgItemTextW(IDC_STATIC_RX, _T(""));
	m_serverWorkMode.SetWindowTextW(_T(""));

	g_dlgHandler = GetSafeHwnd();

	CButton *pOkBtn = (CButton *)GetDlgItem(IDOK);
	if (pOkBtn && pOkBtn->GetSafeHwnd())
	{
		pOkBtn->SetIcon((HICON)LoadImage(AfxGetApp()->m_hInstance,
			MAKEINTRESOURCE(IDI_START),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	}

	CButton *pCancelBtn = (CButton *)GetDlgItem(IDCANCEL);
	if (pCancelBtn && pCancelBtn->GetSafeHwnd())
	{
		pCancelBtn->SetIcon((HICON)LoadImage(AfxGetApp()->m_hInstance,
			MAKEINTRESOURCE(IDI_CANCEL),
			IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
	}



	//m_ServerPort = DEFAULT_PORT;

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void Ciptv_bridgeDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void Ciptv_bridgeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR Ciptv_bridgeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Ciptv_bridgeDlg::addLog(CString sLogLine)
{
	sLogLine += "\r\n";
	CString logs;
	m_logConsole.GetWindowTextW(logs);

	if (logs.GetLength() > 10240) {
		logs = "";
	}
	logs.Append(sLogLine);

	m_logConsole.SetWindowTextW(logs);

	m_logConsole.SetSel(m_logConsole.LineLength());

}


void Ciptv_bridgeDlg::OnBnClickedOk()
{
	UpdateData(true);
	//if ip is empty then work in server mode
	g_serverMode = false;
	g_speedTestMode = 0;

	if (m_ServerPort == 10000)
	{
		g_speedTestMode = 1;
		addLog(_T("Speed test mode"));
		m_ServerPort = DEFAULT_PORT;
	}

	//Get adapter
	int curSel = m_adapterList.GetCurSel();
	if (curSel == -1 && !g_speedTestMode)
	{
		addLog(_T("Please select the adapter connect to STB"));
		return;
	}

	g_adapterIndex = curSel;

	//Get Remote Addr

	m_na.pAddrInfo = &m_nai;
	HRESULT rslt = m_remoteServerAddr.GetAddress(&m_na);
	if (rslt != S_OK)
	{
		//m_remoteServerAddr.DisplayErrorTip();
		g_serverMode = true;
		m_serverWorkMode.SetWindowTextW(_T("Working in server mode"));
		CString s;
		s.Format(_T("No IP specified, will work in server mode:port is %d"), m_ServerPort);
		addLog(s);
	}

	g_serverPort = m_ServerPort;

	if (g_serverMode)
	{
		m_serverWorkMode.SetWindowTextW(_T("Server Mode"));
		CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);
	}
	else
	{
		m_serverWorkMode.SetWindowTextW(_T("Client Mode"));
		CreateThread(NULL, 0, clientThread, &m_nai.IpAddress, 0, NULL);
	}

	if (m_Timer != NULL)
	{
		KillTimer(m_Timer);
	}

	m_Timer = SetTimer(20190929, 1000 * UI_UPDATE_INTEVAL, NULL);

	//CreateThread(NULL, 0, dectectNetworkThread, &m_nai.IpAddress, 0, NULL);

	GetDlgItem(IDOK)->EnableWindow(FALSE);

}


void Ciptv_bridgeDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialogEx::OnCancel();
}


CString Ciptv_bridgeDlg::formatBytes(UINT64 x)
{
	const char *sizes[5] = { "B", "KB", "MB", "GB", "TB" };
	CString sInfo;

	int i;
	double dblByte = (double)x;
	for (i = 0; i < 5 && x >= 1024; i++, x /= 1024)
		dblByte = x / 1024.0;

	sInfo.Format(_T("%.2f %S"), dblByte, sizes[i]);

	return sInfo;
}


 void Ciptv_bridgeDlg::formatTime()
{
	//g_total_seconds += UI_UPDATE_INTEVAL;
	int hour = 0;
	int min = 0;
	int sec = 0;

	UINT64 nSecs = g_total_seconds;
	if (nSecs == 0)
	{
		return;
	}

	// using the time from ^ above, convert 
	// secs to HH:MM:SS format using division
	// and modulus
	hour = nSecs / 3600;
	nSecs = nSecs % 3600;
	min = nSecs / 60;
	nSecs = nSecs % 60;
	sec = nSecs;

	CString mode = _T("Client");
	if (g_serverMode)
	{
		mode = _T("Server");
	}

	CString sInfo;
	sInfo.Format(_T(" IP Bridge %s-- %02d:%02d:%02d"),mode, hour, min, sec);

	this->SetWindowTextW(sInfo);

}

//static int txCount = 0;

BOOL Ciptv_bridgeDlg::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// TODO: Add your specialized code here and/or call the base class
	CString sInfo;
	switch (message)
	{
	case WM_BRIDGE_MSG: 
	{
		char* s = (char *)wParam;
		sInfo.Format(_T("%S"), s);
		addLog(sInfo);
		break;
	}
	case WM_ETH_CABLE_STATUS_CHANGED:
	{
		addLog(_T("Eth cable status changed, will reset the connection"));
		break;
	}
	}

	return CDialogEx::OnWndMsg(message, wParam, lParam, pResult);
}





void Ciptv_bridgeDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent != 20190929)
	{
		CDialogEx::OnTimer(nIDEvent);
		return;
	}

	g_total_seconds += UI_UPDATE_INTEVAL;
	UINT64 totalSecs = g_total_seconds;
	if (totalSecs == 0)
	{
		return;
	}

	CString speed;
#if 1
	if (g_txCount < m_txLastCount) {
		m_txLastCount = 0;
	}

	if (g_rxCount < m_rxLastCount) {
		m_rxLastCount = 0;
	}

	int tx = (int)(g_txCount - m_txLastCount)/ UI_UPDATE_INTEVAL;
	int rx = (int)(g_rxCount - m_rxLastCount)/ UI_UPDATE_INTEVAL;

	m_txLastCount = g_txCount;
	m_rxLastCount = g_rxCount;
	speed.Format(_T("RX:%s/s,TX:%s/s"), formatBytes(rx), formatBytes(tx));

#else		
	speed.Format(_T("RX:%s/s,TX:%s/s"), formatBytes(g_rxCount/ totalSecs), formatBytes(g_txCount / totalSecs));
#endif
	SetDlgItemText(IDC_STATIC_TRANSFER_STATUS, speed);

	CString sInfo;
	sInfo.Format(_T("%s sent,%s received"), formatBytes(g_txCount), formatBytes(g_rxCount));
	SetDlgItemText(IDC_STATIC_TX, sInfo);

	formatTime();
	
}


void Ciptv_bridgeDlg::OnBnClickedCheckCapture()
{
	g_capture = !g_capture;
	if (g_capture == 0)
	{
		addLog(_T("Stop capture package"));
	}
	else
	{
		addLog(_T("Start capture package"));
	}
}






int cfghandler(void* user, const char* section, const char* name,
	const char* value)
{

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	if (MATCH("server", "ip")) {
		
	}
	else if (MATCH("server", "port")) {
		
	}
	else if (MATCH("CmdServer", "ip")) {
		
	}
	else if (MATCH("CmdServer", "port")) {

	}
	else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}

/*
int loadCfg()
{
	configuration config;

	if (ini_parse("iptv_bridge.ini", cfghandler, &config) < 0) {
		printf("Can't load 'test.ini'\n");
		return 1;
	}
	
	return 0;
}
*/

HBRUSH Ciptv_bridgeDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	if (pWnd->GetDlgCtrlID() == IDC_EDIT_LOG)
	{
		pDC->SetBkColor(RGB(0, 255, 255));
		//hbr = m_brBack;
	}

	return hbr;
	//return hbr;
}
