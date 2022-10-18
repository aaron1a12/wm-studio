/*
* Standard headers
*/

#include "standard.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <XMessageBox.h>

/*
* Project headers
*/

#include "resource.h"
#include "wmHttp.h"
#include <json/json.h>
#include "mainWindow.h"
#include "studio.h"
#include <studioDefines.h>

/*
* Global Scope
*/
HWND dlgHwnd;
HWND createAccountLink;
bool bWinHide = false;
bool isUserRemembered = false;
bool isBtnSignEnabled = true;


void onSignInReq(wmHttpResponse response)
{
	isBtnSignEnabled = true;
	EnableWindow(GetDlgItem(dlgHwnd, IDSIGNIN), isBtnSignEnabled);

	// Get state of "Remember Me" checkbox
	int state = (int)SendMessage(GetDlgItem(dlgHwnd, IDC_CHECKREM), BM_GETSTATE, BST_CHECKED, 0);

	if (state == BST_CHECKED)
		Studio.RememberUser = 1;
	else
		Studio.RememberUser = 0;

	// Check if request went through all right

	if (response.status != 200)
	{
		XMSGBOXPARAMS xmb;
		xmb.hInstanceIcon = hInst;
		xmb.dwOptions = xmb.Narrow; // | xmb.VistaStyle
		xmb.nIdIcon = APP_ICON;
		XMessageBox(dlgHwnd, "Could not connect to Wild Montage server. Please verify that you have a connection to escoNet.", "Could not sign in", MB_OK | MB_ICONEXCLAMATION, &xmb);
		//XMessageBox(dlgHwnd, response.text, "Could not sign in", MB_OK | MB_ICONEXCLAMATION, &xmb);

		Studio.LoggedIn = false;
		SetForegroundWindow(hMainWnd);
		ShowWindow(dlgHwnd, SW_SHOW);
	}

	//
	// Login
	//

	Json::Reader reader;
	Json::Value obj;

	bool parseResult = reader.parse(response.text, obj);

	if (parseResult && obj.isObject()) {

		bool jsOk = obj.get("ok", "false").asBool();

		if (jsOk) {
			Studio.LoggedIn = true;

			// Hide the dialog
			ShowWindow(dlgHwnd, SW_HIDE);
			//DestroyWindow(dlgHwnd);
			//EndDialog(dlgHwnd, 1);
			//dlgHwnd = NULL;

			SendMessage(hMainWnd, WMS_LOGGEDIN, 0, 0);
		}
		else {
			// The server sent ok->false so the login failed.
			Studio.LoggedIn = false;

			// Show the login window if hidden
			SetForegroundWindow(hMainWnd);
			ShowWindow(dlgHwnd, SW_SHOW);

			// The "error" key in the JSON response will contain our error message
			const Json::Value jsErrorMsg = obj.get("error", "");

			XMSGBOXPARAMS xmb;
			xmb.hInstanceIcon = hInst;
			xmb.dwOptions = xmb.Narrow; // | xmb.VistaStyle
			xmb.nIdIcon = APP_ICON;
			XMessageBox(dlgHwnd, jsErrorMsg.asCString(), "Could not sign in", MB_OK | MB_ICONEXCLAMATION, &xmb);
		}

	}
}

void validateForm()
{
	//EnableWindow(GetDlgItem(dlgHwnd, IDSIGNIN), TRUE);
	int userLen = (int)SendMessage(GetDlgItem(dlgHwnd, IDC_USERNAME), WM_GETTEXTLENGTH, 0, 0);
	int passLen = (int)SendMessage(GetDlgItem(dlgHwnd, IDC_PASSWORD), WM_GETTEXTLENGTH, 0, 0);


	if (userLen == 0 || passLen == 0) {
		if (isBtnSignEnabled) {
			isBtnSignEnabled = false;
			EnableWindow(GetDlgItem(dlgHwnd, IDSIGNIN), isBtnSignEnabled);
		}
	}
	else {
		if (isBtnSignEnabled==FALSE) {
			isBtnSignEnabled = true;
			EnableWindow(GetDlgItem(dlgHwnd, IDSIGNIN), isBtnSignEnabled);
		}
	}
}

// Message handler for our Main Window. And yes, we're using a dialog box for the UI :P
INT_PTR CALLBACK LoginWindow(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	//int iIndex;

	switch (message)
	{
	case WM_INITDIALOG:
		dlgHwnd = hDlg;


		createAccountLink = GetDlgItem(hDlg, IDC_SYSLINK1);

		// Check remember user
		if (Studio.RememberUser)
			SendMessage(GetDlgItem(hDlg, IDC_CHECKREM), BM_SETCHECK, BST_CHECKED, 0);

		if (Studio.Username.empty() == FALSE && Studio.Password.empty() == FALSE)
		{
			// Attempt to login
			SetDlgItemText(hDlg, IDC_USERNAME, Studio.Username.c_str());
			SetDlgItemText(hDlg, IDC_PASSWORD, Studio.Password.c_str());

			// Set the sign in button disabled
			isBtnSignEnabled = false;
			EnableWindow(GetDlgItem(hDlg, IDSIGNIN), isBtnSignEnabled);

			//MessageBox(NULL, "Make wmHttpRequest.", "WM_INITDIALOG", MB_ICONASTERISK);
			//wmHttpRequest("POST", "https://studio.wildmontage.com/app/login_v2.php", "foo=bar", &onSignInReq);
			wmHttpRequest("GET", "https://studio.wildmontage.com/app/login_v2.php", NULL, &onSignInReq);
		}
		else {
			// Set the sign in button disabled
			isBtnSignEnabled = false;
			SetForegroundWindow(hMainWnd);
			ShowWindow(hDlg, SW_SHOW);
			EnableWindow(GetDlgItem(hDlg, IDSIGNIN), isBtnSignEnabled);
		}

		return (INT_PTR)TRUE;
		break;

	case WM_SHOWWINDOW:
		SetDlgItemText(hDlg, IDC_USERNAME, Studio.Username.c_str());
		SetDlgItemText(hDlg, IDC_PASSWORD, Studio.Password.c_str());
		break;		

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDCLOSE:
			{
				//EndDialog(hDlg, LOWORD(wParam));
				DestroyWindow(hDlg);
				DestroyWindow(hMainWnd);
				return TRUE;
			}
			break;
				
			case IDSIGNIN:
			{
				isBtnSignEnabled = false;
				EnableWindow(GetDlgItem(hDlg, IDSIGNIN), isBtnSignEnabled);

				// Get form values
				HWND inputUser = GetDlgItem(hDlg, IDC_USERNAME);
				HWND inputPassword = GetDlgItem(hDlg, IDC_PASSWORD);

				int userLen = (int)SendMessage(inputUser, WM_GETTEXTLENGTH, 0, 0);
				char szUser[64];
				SendMessageA(inputUser, WM_GETTEXT, (WPARAM)userLen + 1, (LPARAM)szUser);

				int passLen = (int)SendMessage(inputPassword, WM_GETTEXTLENGTH, 0, 0);
				char szPass[64];
				SendMessageA(inputPassword, WM_GETTEXT, (WPARAM)passLen + 1, (LPARAM)szPass);

				Studio.Username.assign(szUser);
				Studio.Password.assign(szPass);

				// Make HTTP GET request
				wmHttpRequest("GET", "https://studio.wildmontage.com/app/login_v2.php", NULL, &onSignInReq);
			}
			break;

			case IDC_USERNAME:
			{
				if (HIWORD(wParam) == EN_CHANGE) {
					validateForm();
				}
			}
			break;
			case IDC_PASSWORD:
			{
				if (HIWORD(wParam) == EN_CHANGE) {
					validateForm();
				}
			}
			break;

		}
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
			case NM_CLICK:   // Fall through to the next case.
			case NM_RETURN:
			{
				PNMLINK pNMLink = (PNMLINK)lParam;
				LITEM   item = pNMLink->item;

				if (wcscmp(item.szID, L"createAccountLink") == 0)
				{
					ShellExecuteW(NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW);
				}

				break;
			}
		}
		break;

	default:
		return FALSE;	// Let system deal with the message
	}
	return TRUE;
}