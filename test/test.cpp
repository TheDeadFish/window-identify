#include "../src/winidentify.h"
#include "../src/windowinfo.h"
#include "resource.h"
#include <stdio.h>
#include <windowsx.h>
#include <process.h>
#include "hexView.cpp"


RECT GetChildRect(HWND hWnd)
{
    RECT Rect; GetWindowRect(hWnd, &Rect);
    MapWindowPoints(HWND_DESKTOP, 
		GetParent(hWnd), (LPPOINT) &Rect, 2);
    return Rect;
}

class ctrlFile
{
public:
	char dllName[260];
	char modName[260];
	HWND hwnd;
	DWORD procId;

	static	
	void RemoveCrap(char *in)
	{	
		int index;
		index = strlen(in);
		while(--index >= 0)
			if(in[index] > ' ')
				break;
		in[index+1] = 0;
	}

	bool read(void)
	{
		FILE* fp = fopen("winInfo.dat", "rb");
		if(fp == NULL)
			return false;
		int size = fread(dllName, 1, 259, fp);
		fclose(fp);
		if(size == 0)
			return false;
		dllName[size] = 0;
		// remove crap
		RemoveCrap(dllName);
		if(GetFileAttributes(dllName) != INVALID_FILE_ATTRIBUTES)
			return true;
		return false;	
	}

	void call(void)
	{
		// write the data to ctrl file
		FILE* fp = fopen("winInfo.dat", "wb");
		fwrite(this, 1, sizeof(*this), fp);
		fclose(fp);
	
		char procIdstr[32];
		sprintf(procIdstr, "%d", procId);
		const char* argv[4] = {"injector.exe", dllName, procIdstr, NULL};
		_execvp("injector.exe", argv);
	}
};

WinIdentify wi; HexView* hexView;
HWND lastHwnd; HANDLE lastProcess;
HICON hIcon; bool mouseOver;

LRESULT CALLBACK ButtonProc(
	HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam
){
	switch(uMsg)
	{
	case WM_PAINT:{
		HDC hDC;
		PAINTSTRUCT Ps;
		hDC = BeginPaint(hwnd, &Ps);
		RECT rc;
		GetClientRect(hwnd, &rc);
		FillRect(hDC, &rc, 
			GetSysColorBrush(COLOR_3DFACE));
		DrawIconEx(hDC, (rc.right-15)/2, (rc.bottom-15)/2,
			hIcon, 0, 0, 0, NULL, DI_NORMAL);			
		if(mouseOver)
			DrawEdge(hDC, &rc, EDGE_RAISED, BF_RECT);
		EndPaint(hwnd, &Ps);
		break;}
		
	case WM_LBUTTONDOWN:
		wi.HwndMark(lastHwnd, true);
		break;
		
	case WM_LBUTTONUP:
		wi.HwndMark(lastHwnd, false);
		SendMessage(GetParent(hwnd), WM_NOTIFY,
			IDC_PARENT, 0);
		break;
		
	case WM_MOUSELEAVE:
		mouseOver = false;
		InvalidateRect(hwnd, NULL, FALSE);
		break;
		
	case WM_MOUSEMOVE:
		if(mouseOver == false)
		{
			mouseOver = true;
			TRACKMOUSEEVENT tm;
			tm.cbSize = sizeof(tm);
			tm.dwFlags = TME_LEAVE;
			tm.hwndTrack = hwnd;
			TrackMouseEvent(&tm);			
			InvalidateRect(hwnd, NULL, FALSE);
		}
		break;
		
	default:
		return DefWindowProc(
			hwnd, uMsg, wParam, lParam);
	}
	return 0;
}

void SetDlgItemHex(HWND hwnd, DWORD ctrlID, DWORD data)
{
	char buff[16]; sprintf(buff, "0x%08X", data);
	SetDlgItemText(hwnd, ctrlID, buff);
}

void hexView_addrcb(void* ctx, DWORD addr, DWORD data)
{
	SetDlgItemHex((HWND)ctx, IDC_ADDR, addr);
	SetDlgItemHex((HWND)ctx, IDC_VALUE, data);
}

void hexView_readcb(void* ctx, DWORD addr, BYTE* data)
{
	DWORD bytesRead = 0;
	ReadProcessMemory(lastProcess, (LPCVOID)addr, data,
		HexView::READ_LEN, &bytesRead);
	for(int i = bytesRead; i < HexView::READ_LEN; i++) { data[i] = 0xAA; }
}

void hexView_resize(HWND hwnd, int width, int height)
{
	RECT rect = GetChildRect(GetDlgItem(hwnd, IDC_ADDRCHG));
	MoveWindow(hexView->hwnd, rect.left, rect.bottom+8,
		(width-8)-rect.left, (height-8)-(rect.bottom+8), 0);
}

void hexView_setAddr(HWND hwnd)
{
	char buff[32];
	GetDlgItemText(hwnd, IDC_ADDR, buff, 32);
	hexView->setAddress(strtol(buff, 0, 16));
}

INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_NOTIFY:
		switch(wParam)
		{
		case IDC_IDENTIFY:{
			NMHDR& nMhdr = *(NMHDR*)lParam;
			lastHwnd = (HWND)nMhdr.code;}
		case IDC_PARENT:{
			CloseHandle(lastProcess);
			WindowInfo wInfo;
			if(lastProcess = wInfo.get(lastHwnd))
			{
				// check for the existance of the control file
				ctrlFile cf;
				if(cf.read())
				{	
					strcpy(cf.modName, wInfo.modName);
					cf.hwnd = lastHwnd;
					cf.procId = wInfo.procId;
					cf.call();
				}
			}
			
			// update the edit control
			SetDlgItemHex(hwndDlg, IDC_HWND, (DWORD)lastHwnd);
			SetDlgItemText(hwndDlg, IDC_CLASS, wInfo.className);
			SetDlgItemHex(hwndDlg, IDC_CTLID, wInfo.ctrlId);
			SetDlgItemHex(hwndDlg, IDC_STYLE, wInfo.style);
			SetDlgItemHex(hwndDlg, IDC_STYLEX, wInfo.exStyle);
			SetDlgItemHex(hwndDlg, IDC_WNDDAT, wInfo.wndData);
			SetDlgItemHex(hwndDlg, IDC_DLGDAT, wInfo.dlgData);
			SetDlgItemHex(hwndDlg, IDC_WNDPROC, wInfo.wndProc);
			SetDlgItemHex(hwndDlg, IDC_DLGPROC, wInfo.dlgProc);
			SetDlgItemText(hwndDlg, IDC_WNDMOD, wInfo.wndProcMod);
			SetDlgItemText(hwndDlg, IDC_DLGMOD, wInfo.dlgProcMod);
			SetDlgItemText(hwndDlg, IDC_PROCESS, wInfo.procName);
			SetDlgItemText(hwndDlg, IDC_INSTANCE, wInfo.modName);
			
			// update extra list
			SetDlgItemHex(hwndDlg, IDC_EXTRA, wInfo.wndExtra);
			HWND hCombo = GetDlgItem(hwndDlg, IDC_WNDDAT2);
			ComboBox_ResetContent(hCombo);
			for(int i = 0; i < wInfo.wndExtra/4; i++) {
				char buff[16]; sprintf(buff, "0x%08X", wInfo.extra[i]);
				ComboBox_AddString(hCombo, buff); }
			ComboBox_SetCurSel(hCombo, 0);
				
			// final steps
			DWORD curAddr = wInfo.wndData;
			if(curAddr == 0) curAddr = wInfo.dlgData;
			if(curAddr == 0) curAddr = wInfo.extra[0];
			hexView->setAddress(curAddr);
			HWND hParent = GetParent(lastHwnd);
			if(hParent)	lastHwnd = hParent;
		}
		break;}
		break;
	
	case WM_INITDIALOG: {
		wi.enumChildren = true;
		wi.replace(hwndDlg, GetDlgItem(hwndDlg, IDC_IDENTIFY));
		
		// setup parent button
		SetWindowLong(GetDlgItem(hwndDlg, IDC_PARENT),
			GWL_WNDPROC, (INT_PTR)&ButtonProc);
		hIcon = (HICON)LoadImageA(GetModuleHandle(NULL),
			"IDI_ICON1", IMAGE_ICON, 0, 0, LR_SHARED);
		mouseOver = false;
		hexView = HexView::create(hwndDlg, hwndDlg, hexView_readcb, hexView_addrcb);
		RECT rect; GetClientRect(hwndDlg, &rect);
		hexView_resize(hwndDlg, rect.right, rect.bottom);
		break; }
	
	case WM_SIZE:
		hexView_resize(hwndDlg, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_COMMAND:
		if(wParam == IDC_ADDRCHG) {
			hexView_setAddr(hwndDlg);  break; }
		if(wParam != IDC_QUIT)
			break;
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;
	default:
		return FALSE;
	}
	return TRUE;
}

int main()
{
	DialogBox(GetModuleHandle(NULL), 
		"IDD_DIALOG1", NULL, &DialogProc);
	printf("%d\n", GetLastError());
	return 0;
}