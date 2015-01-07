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
#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include "message_loop.h"
#include "twain_session.h"

using namespace std;

namespace ctwain{

	////////////////////////////////////////////////////
	// static window registration
	////////////////////////////////////////////////////

	ATOM MessageLoop::class_atom_ = 0;
	HINSTANCE MessageLoop::instance_ = GetModuleHandle(NULL);
	int MessageLoop::window_count_ = 0;
	void MessageLoop::RegisterWindowClass(){
		if (window_count_ == 0){
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
			wcex.lpszClassName = L"TWAIN_INTERNAL_WINDOW";
			wcex.hIconSm = NULL;

			RegisterClassEx(&wcex);
		}
		window_count_++;
	}
	void MessageLoop::UnregisterWindowClass(){
		if (--window_count_ == 0){
			UnregisterClass(MAKEINTATOM(class_atom_), instance_);
		}
	}
	LRESULT CALLBACK MessageLoop::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc;
			hdc = BeginPaint(hWnd, &ps);
			// TODO: Add any drawing code here...
			EndPaint(hWnd, &ps);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		return 0;
	}

	////////////////////////////////////////////////////
	// message loop code
	////////////////////////////////////////////////////
	MessageLoop::MessageLoop(TwainSession* ptwain) : owns_{ true }, twain_{ ptwain }
	{
		condition_variable loopWaiter;
		mutex loopMtx;
		bool threadStarted = false; // don't know enough about condition_variable to know why needs this to work

		thread t{ [this, &loopWaiter, &loopMtx, &threadStarted](){
			unique_lock<mutex> lk(loopMtx);
			threadStarted = true;

			RegisterWindowClass(); // yes ain't thread-safe but any one calling this simultaneously in multiple threads can only blame themselves.
			parent_handle_ = CreateWindow(MAKEINTATOM(class_atom_), L"Twain Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
				NULL, NULL, instance_, NULL);
			/*parent_handle_ = CreateWindowEx(WS_EX_TOOLWINDOW,
			class_name_, L"Twain Window", WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
			HWND_MESSAGE, NULL, instance_, NULL);*/

			if (parent_handle_){
				ShowWindow(parent_handle_, 10);
				UpdateWindow(parent_handle_);

				MSG msg;

				// Main message loop:
				if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
					loopWaiter.notify_all();
					lk.unlock();
					lk.release();
					while (GetMessage(&msg, NULL, 0, 0))
					{
						cout << "got msg " << msg.message << endl;
						if (twain_ == nullptr || !twain_->IsTwainMessage(msg)){
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
					}
					return;
				}
			}

			loopWaiter.notify_all();
			lk.unlock();
			lk.release();
		} };
		unique_lock<mutex> lk(loopMtx);
		while (!threadStarted){
			loopWaiter.wait(lk);
		}
		t.detach();
	}

	MessageLoop::~MessageLoop()
	{
		if (owns_ && parent_handle_ != nullptr){
			DestroyWindow(parent_handle_);
			UnregisterWindowClass();
		}
		parent_handle_ = nullptr;
		twain_ = nullptr;
	}

	void MessageLoop::Post(){
		auto test = 10;

	}
	void MessageLoop::Send(){

	}
}