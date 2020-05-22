
// iptv_bridgeDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"


// Ciptv_bridgeDlg dialog
class Ciptv_bridgeDlg : public CDialogEx
{
// Construction
public:
	Ciptv_bridgeDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IPTV_BRIDGE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	CComboBox m_adapterList;
	NC_ADDRESS m_na;
	NET_ADDRESS_INFO m_nai;

	//UINT64 m_rxCount = 0;
	//UINT64 m_txCount = 0;

	UINT64 m_rxLastCount = 0;
	UINT64 m_txLastCount = 0;

	void formatTime();
	CString formatBytes(UINT64 x);

	UINT_PTR m_Timer = NULL;
public:
	CNetAddressCtrl m_remoteServerAddr;
	CStatic m_serverWorkMode;
	CStatic m_transferStatus;
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

	CEdit m_logConsole;

	void addLog(CString sLogLine);
protected:
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	int m_ServerPort;
	afx_msg void OnBnClickedCheckCapture();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
