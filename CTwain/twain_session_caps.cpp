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
#include <iostream>
#include "build_macros.h"
#include "twain_session.h"
#include "entry_points.h"
#include "message_loop.h"

namespace ctwain{

	TW_UINT16 TwainSession::CapGet(const TW_UINT16 capType, const GetSingleType getType, TW_UINT32& value){
		value = 0;

		TW_CAPABILITY cap;
		cap.Cap = capType;
		cap.ConType = TWON_DONTCARE16;
		cap.hContainer = nullptr;

		TW_UINT16 msg = (getType == GetSingleType::Current) ? MSG_GETCURRENT : MSG_GETDEFAULT;

		auto rc = CallDsm(true, DG_CONTROL, DAT_CAPABILITY, msg, &cap);
		if (rc == TWRC_SUCCESS){
			EntryPoints::Lock(cap.hContainer);

			switch (cap.ConType)
			{
			case TWON_ONEVALUE:
			{
				auto test = static_cast<pTW_ONEVALUE>(cap.hContainer);
				if (test->ItemType < TWTY_FIX32){
					value = test->Item;
				}
				break;
			}
			case TWON_RANGE:
			{
				auto test = static_cast<pTW_RANGE>(cap.hContainer);
				if (test->ItemType < TWTY_FIX32){
					switch (getType){
					case GetSingleType::Current:
						value = test->CurrentValue;
						break;
					case GetSingleType::Default:
						value = test->DefaultValue;
						break;
					}
				}
				break;
			}
			case TWON_ENUMERATION:
			{
				auto test = static_cast<pTW_ENUMERATION>(cap.hContainer);
				if (test->ItemType < TWTY_FIX32){
					switch (getType){
					case GetSingleType::Current:
						value = test->ItemList[test->CurrentIndex];
						break;
					case GetSingleType::Default:
						value = test->ItemList[test->DefaultIndex];
						break;
					}
				}
				break;
			}
			case TWON_ARRAY: // not logical
				break;
			}

			EntryPoints::Unlock(cap.hContainer);
		}

		if (cap.hContainer){
			EntryPoints::Free(cap.hContainer);
		}
		return rc;
	}
}