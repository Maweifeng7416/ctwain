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

#ifndef REGISTERED_WINDOW_H_
#define REGISTERED_WINDOW_H_

namespace ctwain{
#ifdef TWH_CMP_MSC
	/// <summary>
	/// Wraps windows registration and creation code
	/// and keeps track of call counts to only unregister at 0.
	/// This should not be used by consumers.
	/// </summary>
	class RegisteredWindow{
	public:
		/// <summary>
		/// Creates a window that's suitable for the internal Windows message loop.
		/// </summary>
		/// <returns>Window handle</returns>
		static HWND CreateMyWindow();
		/// <summary>
		/// Destroys the window from CreateMyWindow(). This must be the method used to
		/// cleanup HWND from CreateMyWindow().
		/// </summary>
		/// <param name="handle">The handle.</param>
		static void DestroyMyWindow(HWND handle);

		static void Register();
		static void Unregister();
	private:
		static HINSTANCE instance_;
		static LPCWSTR class_name_;
		static int count_;
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};
#endif
}

#endif //REGISTERED_WINDOW_H_