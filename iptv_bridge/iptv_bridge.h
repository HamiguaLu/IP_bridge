
// iptv_bridge.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Ciptv_bridgeApp:
// See iptv_bridge.cpp for the implementation of this class
//

class Ciptv_bridgeApp : public CWinApp
{
public:
	Ciptv_bridgeApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Ciptv_bridgeApp theApp;