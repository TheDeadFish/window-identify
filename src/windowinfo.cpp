#include "windowinfo.h"
#include <psapi.h>
#include <stdio.h>

#ifndef _STRING_MACRO_
#define _STRING_MACRO_
 #define STRING1(x) #x
 #define STRING(x) STRING1(x)
#endif

struct INJDATA
{
	INJDATA(bool isDialog, HWND hwnd)
	{
		HMODULE hModule = GetModuleHandleA("user32.dll");
		gwlA = (typeof(&GetWindowLongPtr))
			GetProcAddress(hModule, STRING(GetWindowLongPtrA));
		gwlW = (typeof(&GetWindowLongPtr))
			GetProcAddress(hModule, STRING(GetWindowLongPtrW));
		gcLong = (typeof(&GetClassLongPtr))
			GetProcAddress(hModule, STRING(GetClassLongPtrW));
		this->isDialog = isDialog;
		this->hwnd = hwnd;
		wndProc = 0;
		dlgProc = 0;
	}
	typeof(&GetWindowLongPtr) gwlA;
	typeof(&GetWindowLongPtr) gwlW;
	typeof(&GetClassLongPtr) gcLong;
	HWND hwnd; bool isDialog;
	LONG_PTR wndProc, dlgProc;
	int wndExtra; LONG_PTR extra[40];
};

#define THREADFUNC_SIZE 256
static DWORD WINAPI ThreadFunc (INJDATA *pData)
{
	// get window procedures
	pData->wndProc = pData->gwlA(pData->hwnd, GWLP_WNDPROC);
	if(pData->wndProc < 0) 
		pData->wndProc = pData->gwlW(pData->hwnd, GWLP_WNDPROC);
	if(pData->isDialog == true) {
		pData->dlgProc = pData->gwlA(pData->hwnd, DWLP_DLGPROC);
		if(pData->dlgProc < 0)
			pData->dlgProc = pData->gwlW(pData->hwnd, DWLP_DLGPROC); 
	}
	
	// get window extra
	pData->wndExtra = pData->gcLong(pData->hwnd, GCL_CBWNDEXTRA);
	for(int i = 0; i < pData->wndExtra; i++)
		pData->extra[i] = pData->gwlW(pData->hwnd, i*4);
	return 0;
}

static void
procModInfo(HANDLE hProcess, LONG_PTR& wndProc)
{
	char* procMod = (char*)(&wndProc+1);
	procMod[0] = '\0';
	if(wndProc != 0)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if(VirtualQueryEx( hProcess, (void*)wndProc, &mbi, sizeof(mbi)))
		{
			GetModuleFileNameExA(hProcess,
				(HMODULE)mbi.AllocationBase, procMod, MAX_PATH);
		}
	}
}

HANDLE WindowInfo::get(HWND hwnd)
{
	procId = 0;
	procName[0] = '\0';
	modName[0] = '\0';
	className[0] = '\0';

	// get process handle
	GetWindowThreadProcessId(hwnd, &procId);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, 0, procId);
	if(hProcess == NULL) return hProcess;
		
	// get wndProc/dlgProc address
	GetClassNameA(hwnd, className, MAX_PATH);	
	INJDATA injData(!lstrcmpA(className, "#32770"), hwnd);
	LPVOID pMemRemote = VirtualAllocEx(
		hProcess, 0, sizeof(INJDATA)+THREADFUNC_SIZE,
		MEM_COMMIT, PAGE_EXECUTE_READWRITE );
	if(pMemRemote != NULL)
	{
		WriteProcessMemory( hProcess, pMemRemote, &injData, sizeof(INJDATA), 0);
		WriteProcessMemory( hProcess, pMemRemote+sizeof(INJDATA),
			(void*)&ThreadFunc, THREADFUNC_SIZE, 0);
		HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
			LPTHREAD_START_ROUTINE( pMemRemote+sizeof(INJDATA) ), pMemRemote, 0 , NULL);
		WaitForSingleObject(hThread, INFINITE);
		ReadProcessMemory( hProcess, pMemRemote, &injData, sizeof(INJDATA), 0);
		CloseHandle(hThread);
	}
	wndProc = injData.wndProc; dlgProc = injData.dlgProc;
	wndExtra = injData.wndExtra; memcpy(extra, injData.extra, 4*40);
	VirtualFreeEx(hProcess, pMemRemote, 0, MEM_RELEASE );
	
	// get remaining information
	HMODULE hModule = (HMODULE)GetWindowLongPtr(hwnd, GWL_HINSTANCE);
	GetModuleFileNameExA(hProcess, 0, procName, MAX_PATH);
	GetModuleFileNameExA(hProcess, hModule, modName, MAX_PATH);
	ctrlId = GetWindowLong(hwnd, GWLP_ID);
	procModInfo(hProcess, wndProc);
	procModInfo(hProcess, dlgProc);
	style = GetWindowLong(hwnd, GWL_STYLE);
	exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	wndData = GetWindowLong(hwnd, GWL_USERDATA);
	dlgData = GetWindowLong(hwnd, DWL_USER);	
	return hProcess;
}
