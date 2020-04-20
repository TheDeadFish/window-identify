#ifndef _HEXVIEW_H_
#define _HEXVIEW_H_
#include <windows.h>
#include <stdio.h>
#define min(a, b) (((a) < (b)) ? (a) : (b))

struct HexView
{
	typedef SIZE_T ADDR_T;



	// public interface
	HWND hwnd; enum { READ_LEN = 19 };
	typedef void (*ReadDataCb)(void* ctx, ADDR_T addr, BYTE* data);
	typedef void (*AddrChngCb)(void* ctx, ADDR_T addr, DWORD data);
	static HexView* create(HWND hwnd, void* ctx,
		ReadDataCb datacb_, AddrChngCb addrcb_);
	static HexView* replace(HWND hwnd, void* ctx,
		ReadDataCb datacb_, AddrChngCb addrcb_);
	void setAddress(ADDR_T newAddr);
		
//private:
	RECT rect; void* ctx;
	ReadDataCb datacb; AddrChngCb addrcb;
	ADDR_T scrnAddr, curAddr; DWORD nRows, rowHeight;
	SIZE textSz; void resize(WINDOWPOS* wndPos);
	void keybPress(WPARAM vKey); 
	void clampScrn(void); void redraw(HDC hdc);
	void setColor(HDC hdc, char sel);
	void drawHexLine(HDC hdc, int x, int y,
		char focusCol, int cursor, ADDR_T addr, BYTE* data);
	static LRESULT CALLBACK WndProc(HWND hwnd,
		UINT msg, WPARAM wParam, LPARAM lParam);
};

#if __INCLUDE_LEVEL__ == 0

HexView* HexView::create(HWND hwnd_, void* ctx_,
	ReadDataCb datacb_, AddrChngCb addrcb_)
{
	return replace(CreateWindow(TEXT("Message"), NULL, WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0, hwnd_, 0, 0, 0), ctx_, datacb_, addrcb_);
}

HexView* HexView::replace(HWND hwnd_, void* ctx_,
	ReadDataCb datacb_, AddrChngCb addrcb_)
{
	// initialize object
	HexView* This = (HexView*)malloc(sizeof(HexView));
	HDC hdc = GetDC(hwnd_);
	SelectObject(hdc, GetStockObject(OEM_FIXED_FONT));
	GetTextExtentPoint32A(hdc, " ", 1, &This->textSz);
	ReleaseDC(hwnd_, hdc);
	This->hwnd = hwnd_; This->ctx = ctx_; 
	This->datacb = datacb_; This->addrcb = addrcb_;
	This->scrnAddr = 0; This->curAddr = 0;		
	
	// update window
	SendMessage(hwnd_, WM_DESTROY, 0, 0);
	SendMessage(hwnd_, WM_NCDESTROY, 0, 0);
	SetWindowLongPtr(hwnd_, GWLP_WNDPROC, (LONG_PTR)WndProc);
	SetWindowLongPtr(hwnd_, GWLP_USERDATA, (LONG_PTR)This);
	return This;
}

LRESULT CALLBACK HexView::WndProc(HWND hwnd,
	UINT msg, WPARAM wParam, LPARAM lParam)
{
	HexView* This = (HexView*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch(msg) {
	case WM_PAINT: { PAINTSTRUCT ps; HDC hdc = BeginPaint(hwnd, &ps); 
		This->redraw(hdc); EndPaint(hwnd, &ps); break; }
	case WM_WINDOWPOSCHANGING:
		This->resize((WINDOWPOS*)lParam); break;
	case WM_SIZE: case WM_SETFOCUS: case WM_KILLFOCUS:
		InvalidateRect(hwnd, NULL, TRUE); break;
	case WM_KEYDOWN: This->keybPress(wParam); break;
	case WM_LBUTTONDOWN: SetFocus(hwnd); break;
	case WM_NCDESTROY: free(This);	break;
	case WM_GETDLGCODE: return DLGC_WANTARROWS;
	default: return DefWindowProc(hwnd, msg, wParam, lParam);
	} return 0;
}

void HexView::resize(WINDOWPOS* wndPos)
{
	rect.left = 0; rect.top = 0;
	rect.right = wndPos->cx = min(wndPos->cx, 
		(rect.left+4) + textSz.cx*77);
	nRows = (wndPos->cy-4)/textSz.cy;
	rect.bottom = rect.top+4 + textSz.cy*nRows;
	wndPos->cy = rect.bottom;
	this->clampScrn();
}

void HexView::setAddress(SIZE_T newAddr)
{
	curAddr = newAddr; 
	scrnAddr = newAddr&~15;
	InvalidateRect(hwnd, NULL, FALSE);
}

void HexView::clampScrn(void)
{
	ADDR_T limitMax = curAddr & ~15;
	if(scrnAddr > limitMax) scrnAddr = limitMax;
	ADDR_T limitMin = limitMax - (nRows-1)*16;
	if(scrnAddr < limitMin) scrnAddr = limitMin;
}

void HexView::setColor(HDC hdc, char sel)
{
	static const COLORREF bakColor[3] = {RGB(0,0,0), RGB(192,192,192), RGB(255,255,255)};
	static const COLORREF forColor[3] = {RGB(0,255,0), RGB(192,0,192), RGB(255,0,255)};
	SetTextColor(hdc, forColor[sel]); SetBkColor(hdc, bakColor[sel]);
}

void HexView::keybPress(WPARAM vKey)
{
	switch(vKey) { default: return; 
	case VK_LEFT: curAddr -= 1; break; case VK_RIGHT: curAddr += 1; break;
	case VK_UP: curAddr -= 16; break; case VK_DOWN: curAddr += 16; break;
	case VK_PRIOR: curAddr -= 16*nRows; scrnAddr -= 16*nRows; break;
	case VK_NEXT: curAddr += 16*nRows; scrnAddr += 16*nRows; break; }
	clampScrn(); InvalidateRect(hwnd, NULL, FALSE);
}

void HexView::redraw(HDC hdc)
{
	BYTE data[READ_LEN];
	SelectObject(hdc, GetStockObject(OEM_FIXED_FONT));
	SetTextAlign(hdc, GetTextAlign(hdc) | TA_UPDATECP);
	char focusCol = (GetFocus() == hwnd) ? 2 : 1;
	for(int i = 0; i < nRows; i++) {
		DWORD lineAddr = scrnAddr + i*16;
		datacb(ctx, lineAddr, data);
		unsigned cursor = curAddr-lineAddr;	if(cursor < 16) {
			addrcb(ctx, curAddr, *(int*)(data+cursor)); }
		drawHexLine(hdc, rect.left+2, (rect.top+2) +
			i*textSz.cy, focusCol, cursor, lineAddr, data); 
	} DrawEdge(hdc, &rect, BDR_SUNKENINNER | BDR_SUNKENOUTER, BF_RECT);	
}

#include <conio.h>

void HexView::drawHexLine(HDC hdc, int x, int y,
	char focusCol, int cursor, ADDR_T addr, BYTE* data)
{
	char buff[16];
	MoveToEx(hdc, x, y, 0); this->setColor(hdc, 0); 
	TextOutA(hdc, 0, 0, buff, sprintf(buff, "%08X |", addr));
	for(int i = 0; i < 16; i++) {
		TextOutA(hdc, 0, 0, (i == 8) ? "-" : " ", 1);
		if(i == cursor) this->setColor(hdc, focusCol); 
		TextOutA(hdc, 0, 0, buff, sprintf(buff, "%02X", data[i])); 
		if(i == cursor) this->setColor(hdc, 0); }
	TextOutA(hdc, 0, 0, " | ", 3);
	for(int i = 0; i < 16; i++) { 
		this->setColor(hdc, (i == cursor) ? focusCol : 0); 
		buff[0] = data[i]; TextOutA(hdc, 0, 0, buff, 1); }
}

#endif
#endif
