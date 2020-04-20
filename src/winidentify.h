#ifndef _WINIDENTIFY_H_
#define _WINIDENTIFY_H_
#include <windows.h>

class WinIdentify
{
public:
	WinIdentify();
	void HwndMark(HWND hwnd, bool draw);
	HWND create(HWND parent, RECT& rc);
	HWND replace(HWND parent, HWND child);
	void destroy(void);
	void setCtrlId(short id);
	bool enumChildren;
	HWND hwnd;
	
	struct MNWND { NMHDR nmh; HWND hwnd; };
	static HWND getNotify(LPARAM lParam) {
		return ((MNWND*)lParam)->hwnd; }
	
private:
	class WinCollide
	{
	public:
		bool enumChildren;
		HWND topHwnd;
		void Update();
		HWND Collide(POINT poit);
	private:
		HWND hwnd[4096];
		RECT rect[4096];
		int WinCount;
		static BOOL CALLBACK WinCollProc(HWND _hwnd, LPARAM lParam);
	};

	// private members
	HWND lastHwnd;
	HICON hIcon;
	bool mouseOver;
	WinCollide* winCol;
	
	// static members
	static HINSTANCE hInst;
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);	
};

inline
WinIdentify::WinIdentify()
{
	enumChildren = false;
}

#endif
