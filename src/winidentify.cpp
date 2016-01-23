#include "winidentify.h"
#include <stdio.h>
#include <stdlib.h>

BOOL CALLBACK  WinIdentify::WinCollide::WinCollProc(HWND _hwnd, LPARAM lParam)
{
	WinCollide* wc = (WinCollide*)lParam;
	if(IsWindowVisible(_hwnd))
	{
		if((wc->enumChildren)&&(_hwnd != wc->topHwnd))
			EnumChildWindows(_hwnd, &WinCollProc, lParam);
		wc->hwnd[wc->WinCount] = _hwnd;
		GetWindowRect(_hwnd, &wc->rect[wc->WinCount]);
		wc->WinCount++;
	}
	return TRUE;
}

void WinIdentify::WinCollide::Update(void)
{
	WinCount = 0;
	EnumWindows(&WinCollProc, (LPARAM)this);
}

HWND WinIdentify::WinCollide::Collide(POINT poit)
{
	// Check for encapsulation of the point
	for(int i = 0; i < WinCount; i++)
	{
		if((rect[i].top < poit.y)&&(rect[i].bottom > poit.y)
		&&(rect[i].left < poit.x)&&(rect[i].right > poit.x))
			return (hwnd[i] != topHwnd) ? hwnd[i] : NULL;
	}
	return NULL;
}

LRESULT CALLBACK WinIdentify::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WinIdentify* obj;
	if(msg == WM_CREATE)
	{
		CREATESTRUCT& cs = *(CREATESTRUCT*)lParam;
		SetWindowLong(hwnd, GWL_USERDATA, (LONG)cs.lpCreateParams);
		obj = (WinIdentify*)cs.lpCreateParams;
		
		// setup object
		obj->hIcon = (HICON)LoadImage(hInst, "WinIdentify_Icon", IMAGE_ICON, 0, 0, 0);
		obj->lastHwnd = NULL;
		obj->mouseOver = false;
		obj->winCol = NULL;
		return TRUE;
	}
	
	obj = (WinIdentify*)GetWindowLong(hwnd, GWL_USERDATA);
	switch(msg)
	{
	case WM_PAINT:{
		HDC hDC;
		PAINTSTRUCT Ps;
		hDC = BeginPaint(hwnd, &Ps);
		RECT rc;
		GetClientRect(hwnd, &rc);
		if(obj->mouseOver)
			DrawEdge(hDC, &rc, EDGE_RAISED, BF_RECT);
		DrawIconEx(hDC, (rc.right-15)/2, (rc.bottom-15)/2,
			obj->hIcon, 0, 0, 0, NULL, DI_NORMAL);
		EndPaint(hwnd, &Ps);
		break;}
		
	case WM_MOUSELEAVE:
		obj->mouseOver = false;
		InvalidateRect(hwnd, NULL, TRUE);
		break;
		
	case WM_MOUSEMOVE:
		if(obj->mouseOver == false)
		{
			obj->mouseOver = true;
			TRACKMOUSEEVENT tm;
			tm.cbSize = sizeof(tm);
			tm.dwFlags = TME_LEAVE;
			tm.hwndTrack = hwnd;
			TrackMouseEvent(&tm);
			InvalidateRect(hwnd, NULL, TRUE);
		}
		if(obj->winCol != NULL)
		{
			POINT pos = {short(LOWORD(lParam)), short(HIWORD(lParam))};
			ClientToScreen(hwnd, &pos);
			obj->winCol->Update();
			obj->HwndMark(obj->winCol->Collide(pos), true);
		}
		break;
		
	case WM_LBUTTONDOWN:
		if(obj->winCol != NULL)
			free(obj->winCol);
		obj->winCol = (WinCollide*)malloc(sizeof(WinCollide));
		if(obj->winCol != NULL)
		{
			obj->winCol->enumChildren = obj->enumChildren;
			obj->winCol->topHwnd = GetAncestor(hwnd, GA_ROOT);
			SetCapture(hwnd);
		}
		break;
		
	case WM_LBUTTONUP:
		if(obj->winCol != NULL)
		{
			ReleaseCapture();
			POINT pos = {short(LOWORD(lParam)), short(HIWORD(lParam))};
			ClientToScreen(hwnd, &pos);
			obj->winCol->Update();
			HWND selHwnd = obj->winCol->Collide(pos);
			obj->HwndMark(selHwnd, false);
			free(obj->winCol);
			obj->winCol = NULL;
			
			// send notifacation
			if(selHwnd)
			{
				NMHDR nMhdr;
				nMhdr.hwndFrom = hwnd;
				nMhdr.idFrom = GetDlgCtrlID(hwnd);
				nMhdr.code = (UINT)selHwnd;
				SendMessage( GetParent(hwnd), WM_NOTIFY,
					nMhdr.idFrom, (LPARAM)&nMhdr);
			}
		}
	
	case WM_DESTROY:
		if(obj->winCol != NULL)
			free(obj->winCol);
		break;
		
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void WinIdentify::HwndMark(HWND curHwnd, bool draw)
{
	if((lastHwnd != NULL)&&((draw == false)||(curHwnd != lastHwnd)))
	{
		SetWindowPos(lastHwnd, 0, 0, 0, 0, 0,  SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE |SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOCOPYBITS);
		lastHwnd = 0;
	}	
	if((curHwnd != NULL)&&(draw == true))
	{
		lastHwnd = curHwnd;
		HDC hdc = GetWindowDC( curHwnd );
		// create pen 
		HPEN hpen = CreatePen( PS_SOLID, 4, RGB(255, 0, 0) );
		SelectObject(hdc, hpen);
		SelectObject(hdc, GetStockObject(NULL_BRUSH));
		// draw the rectangle
		RECT rect;
		GetWindowRect(curHwnd, &rect);
		Rectangle(hdc, 2, 2, (rect.right - rect.left)-2, (rect.bottom - rect.top)-2);
		ReleaseDC( curHwnd, hdc);
		DeleteObject( hpen );
	}	
}

HINSTANCE WinIdentify::hInst = 0;
HWND WinIdentify::create(HWND parent, RECT& rc)
{
	hwnd = NULL;
	if(hInst == 0)
	{
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery( (void*)&hInst, &mbi, sizeof(mbi) );
		hInst = (HINSTANCE)mbi.AllocationBase;
		
		WNDCLASS wc = {0};
		wc.lpfnWndProc 		= &WndProc;
		wc.hInstance 		= hInst;
		wc.hCursor     		= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground 	= (HBRUSH)(COLOR_WINDOW);
		wc.lpszClassName 	= "WinIdentify";
		if(RegisterClass(&wc) == 0)
		{
			hInst = 0;
			return NULL;
		}
	}
	hwnd = CreateWindow("WinIdentify", "WinIdentify", WS_CHILD | WS_VISIBLE,
		rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, parent, NULL, hInst, this);
	return hwnd;
}

HWND WinIdentify::replace(HWND parent, HWND child)
{
	RECT rc;
	GetWindowRect(child, &rc);
	ScreenToClient(parent, ((POINT*)&rc)+0);
	ScreenToClient(parent, ((POINT*)&rc)+1);
	short ctrlId = GetDlgCtrlID(child);
	DestroyWindow(child);
	WinIdentify::create(parent, rc);
	setCtrlId(ctrlId);
	return hwnd;
}

void WinIdentify::destroy(void)
{
	DestroyWindow(hwnd);
}

void WinIdentify::setCtrlId(short ctrlId)
{
	SetWindowLong(hwnd, GWL_ID, ctrlId);
}
