// The MIT License (MIT)
// Copyright (c) 2015 Yin-Chun Wang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy 
// of this software and associated documentation files (the "Software"), to deal 
// in the Software without restriction, including without limitation the rights to 
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
// of the Software, and to permit persons to whom the Software is furnished 
// to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE 
// OR OTHER DEALINGS IN THE SOFTWARE.
//
#include "stdafx.h"
#include "registered_window.h"


namespace ctwain{

#ifdef TWH_CMP_MSC
	LPCWSTR RegisteredWindow::class_name_ = L"TWAIN_INTERNAL_WINDOW";
	HINSTANCE RegisteredWindow::instance_ = GetModuleHandle(NULL);
	int RegisteredWindow::count_ = 0;

	HWND RegisteredWindow::CreateMyWindow(){
		Register();
		auto hwnd = CreateWindow(class_name_, L"Twain Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
			NULL, NULL, instance_, NULL);
		/*auto hwnd = CreateWindowEx(WS_EX_TOOLWINDOW,
			class_name_, L"Twain Window", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
			HWND_MESSAGE, NULL, instance_, NULL);*/
		if (hwnd){
			ShowWindow(hwnd, 10);
			UpdateWindow(hwnd);
		}
		return hwnd;
	}
	void RegisteredWindow::DestroyMyWindow(HWND handle){
		if (handle){
			DestroyWindow(handle);
		}
		Unregister();
	}
	void RegisteredWindow::Register(){
		if (count_ == 0){
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX);

			wcex.style = CS_HREDRAW | CS_VREDRAW;
			wcex.lpfnWndProc = WndProc;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = 0;
			wcex.hInstance = instance_;
			wcex.hIcon = NULL;
			wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
			wcex.lpszMenuName = NULL;
			wcex.lpszClassName = class_name_;
			wcex.hIconSm = NULL;

			RegisterClassEx(&wcex);
		}
		count_++;
	}
	void RegisteredWindow::Unregister(){
		if (--count_ == 0){
			UnregisterClass(class_name_, instance_);
		}
	}
	LRESULT CALLBACK RegisteredWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}
#endif
}