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
#include "message_loop.h"
#include "registered_window.h"

namespace ctwain{

	MessageLoop::MessageLoop() :parent_handle_{ RegisteredWindow::CreateMyWindow() }, owns_{ true }
	{
		if (parent_handle_){
			ShowWindow(parent_handle_, 10);
			UpdateWindow(parent_handle_);
			// todo: run message loop in another thread
		}
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