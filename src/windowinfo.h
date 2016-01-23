#ifndef _WINDOWINFO_H_
#define _WINDOWINFO_H_
#include <windows.h>

class WindowInfo
{
public:
	DWORD procId;
	char procName[260];
	char modName[260];
	char className[120];		
	LONG ctrlId;	
	LONG_PTR wndProc;	
	char wndProcMod[260];
	LONG_PTR dlgProc;
	char dlgProcMod[260];
	DWORD style, exStyle;
	DWORD wndData, dlgData;
	DWORD wndExtra; 
	LONG_PTR extra[40];
	HANDLE get(HWND hwnd);
};

#endif
