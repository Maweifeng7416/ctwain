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
#include <memory>
#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>
#include "message_loop.h"
#include "registered_window.h"
#include "twain_session.h"

using namespace std;

namespace ctwain{

	MessageLoop::MessageLoop(TwainSession* ptwain) : owns_{ true }, twain{ ptwain }, ready{ false }
	{

		cout << "in loop ctor" << endl;
		thread t{ [this](){
			cout << "in loop thread" << endl;
			unique_lock<mutex> lk(m);
			ready = true;
			parent_handle_ = RegisteredWindow::CreateMyWindow();
			if (parent_handle_){
				ShowWindow(parent_handle_, 10);
				UpdateWindow(parent_handle_);

				MSG msg;

				// Main message loop:
				if (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE)){
					cout << "peek ok, notifying" << endl;
					waiter.notify_all();
					lk.unlock();
					lk.release();
					while (GetMessage(&msg, NULL, 0, 0))
					{
						cout << "got msg " << msg.message << endl;
						if (!twain->IsTwainMessage(msg)){
							TranslateMessage(&msg);
							DispatchMessage(&msg);
						}
					}
				}
				else{
					cout << "peek failed" << endl;
					waiter.notify_all();
					lk.unlock();
					lk.release();
				}
			}
			else{
				cout << "create handle failed" << endl;
				waiter.notify_all();
				lk.unlock();
				lk.release();
			}
		} };
		unique_lock<mutex> lk(m);
		while (!ready){
			cout << "waiting for condition" << endl;
			waiter.wait(lk);
		}
		cout << "exiting loop ctor" << endl;
		t.detach();
	}

	MessageLoop::~MessageLoop()
	{
		if (owns_ && parent_handle_ != nullptr){
			RegisteredWindow::DestroyMyWindow(parent_handle_);
		}
		parent_handle_ = nullptr;
	}

	void MessageLoop::Post(){
		auto test = 10;

	}
	void MessageLoop::Send(){

	}
}