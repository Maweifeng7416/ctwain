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

#ifndef MESSAGE_LOOP_H_
#define MESSAGE_LOOP_H_

namespace ctwain{

	class TwainSession;

	/// <summary>
	/// A base interface for a message loop for TWAIN to use.
	/// </summary>
	class MessageLoop {
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="MessageLoop"/> class
		/// with an internal message loop.
		/// </summary>
		MessageLoop(TwainSession*);

		/// <summary>
		/// Initializes a new instance of the <see cref="MessageLoop"/> class
		/// using an existing window handle.
		/// </summary>
		/// <param name="handle">The handle.</param>
		MessageLoop(HWND handle) :parent_handle_{ handle }{};

		// no copy ctor
		MessageLoop(const MessageLoop&) = delete;
		// no copy assign
		MessageLoop& operator=(const MessageLoop&) = delete;

		// move ctor
		MessageLoop(MessageLoop&& other) :parent_handle_{ other.parent_handle_ }, owns_{ other.owns_ }{
			other.parent_handle_ = nullptr;
		}
		// move assign
		MessageLoop& operator=(MessageLoop&& other){
			if (this != &other)
			{
				parent_handle_ = other.parent_handle_;
				owns_ = other.owns_;
				other.parent_handle_ = nullptr;
			}
			return *this;
		}

		~MessageLoop();

		void Send();
		void Post();

		/// <summary>
		/// Gets the parent handle to the message loop.
		/// </summary>
		HWND parent_handle() { return parent_handle_; }

	protected:
		HWND parent_handle_ = nullptr;
		bool owns_ = false;

		TwainSession* twain_;

		static void RegisterWindowClass();
		static void UnregisterWindowClass();
		static HINSTANCE instance_;
		static ATOM class_atom_;
		static int window_count_;
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
}

#endif //MESSAGE_LOOP_H_