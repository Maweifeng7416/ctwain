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
#include "message_loop.h"
#include "registered_window.h"
namespace ctwain{

	////////////////////////////////////////////
	// Windows message loop use
	////////////////////////////////////////////
#ifdef TWH_CMP_MSC

	


	WindowsMessageLoop::WindowsMessageLoop()
	{
		auto hInstance = GetModuleHandle(NULL);
		window_handle_ = RegisteredWindow::CreateMyWindow();

	}


	WindowsMessageLoop::~WindowsMessageLoop()
	{
		RegisteredWindow::DestroyMyWindow(window_handle_);
	}

	void WindowsMessageLoop::Post(){

	}
	void WindowsMessageLoop::Send(){

	}

#endif

}