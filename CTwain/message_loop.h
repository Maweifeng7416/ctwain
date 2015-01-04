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

	class MessageLoop
	{
	public:
		MessageLoop();
		virtual ~MessageLoop();
		virtual void Send() = 0;
		virtual void Post() = 0;
	};


#ifdef TWH_CMP_MSC
	class WindowsMessageLoop : public MessageLoop
	{
	public:
		WindowsMessageLoop();
		~WindowsMessageLoop();
		void Send();
		void Post();
		HWND window_handle(){ return window_handle_; }

	private:
		HWND window_handle_ = nullptr;
	};
#endif
}

#endif //MESSAGE_LOOP_H_