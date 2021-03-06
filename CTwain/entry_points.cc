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
#include "entry_points.h"
#include "build_macros.h"

namespace ctwain{

	HMODULE EntryPoints::dsm_module_ = nullptr;
	DSMENTRYPROC EntryPoints::dsm_entry_ = nullptr;
	TW_ENTRYPOINT EntryPoints::memory_entry_{ 0 };

	bool EntryPoints::InitializeDSM(){
		if (!dsm_module_){

#ifdef TWH_CMP_MSC
			dsm_module_ = LOADLIBRARY(L"twaindsm.dll");
#elif
			dsm_module_ = LOADLIBRARY("/usr/local/lib/libtwaindsm.so");
#endif
			if (dsm_module_){
				dsm_entry_ = (DSMENTRYPROC) LOADFUNCTION(dsm_module_, "DSM_Entry");
				if (dsm_entry_){
					return true;
				}
				else{
					UNLOADLIBRARY(dsm_module_);
					dsm_module_ = nullptr;
				}
			}
		}
		return dsm_entry_ != nullptr;
	}


	void EntryPoints::UninitializeDSM() {
		dsm_entry_ = nullptr;
		if (dsm_module_) {
			UNLOADLIBRARY(dsm_module_);
			dsm_module_ = nullptr;
		}
	}

	TW_UINT16 EntryPoints::DSM_Entry(pTW_IDENTITY orig, pTW_IDENTITY dest, TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData) {
		if (dsm_entry_) {
			return dsm_entry_(orig, dest, DG, DAT, MSG, pData);
		}
		return TWRC_FAILURE;
	}

	void EntryPoints::UpdateMemoryEntry(pTW_IDENTITY appId){
		if (appId){
			if ((appId->SupportedGroups & DF_DSM2) == DF_DSM2){
				TW_ENTRYPOINT ep{ 0 };
				ep.Size = sizeof(TW_ENTRYPOINT);
				auto rc = DSM_Entry(appId, nullptr, DG_CONTROL, DAT_ENTRYPOINT, MSG_GET, &ep);
				if (rc == TWRC_SUCCESS){
					memory_entry_ = ep;
					return;
				}
			}
		}
		memory_entry_ = { 0 };
	}
	TW_HANDLE EntryPoints::Alloc(TW_UINT32 size){
		if (memory_entry_.DSM_MemAllocate){
			return memory_entry_.DSM_MemAllocate(size);
		}
#ifdef TWH_CMP_MSC
		return GlobalAlloc(GPTR, size);
#else
		return malloc(size);
#endif
	}

	void EntryPoints::Free(TW_HANDLE handle){
		if (memory_entry_.DSM_MemFree){
			memory_entry_.DSM_MemFree(handle);
			return;
		}
#ifdef TWH_CMP_MSC
		GlobalFree(handle);
#else
		free(handle);
#endif
	}

	TW_MEMREF EntryPoints::Lock(TW_HANDLE handle){
		if (memory_entry_.DSM_MemLock){
			return memory_entry_.DSM_MemLock(handle);
		}
#ifdef TWH_CMP_MSC
		return GlobalLock(handle);
#else
		return handle;
#endif
	}

	void EntryPoints::Unlock(TW_HANDLE handle){
		if (memory_entry_.DSM_MemUnlock){
			memory_entry_.DSM_MemUnlock(handle);
			return;
		}
#ifdef TWH_CMP_MSC
		GlobalUnlock(handle);
#endif
	}
}