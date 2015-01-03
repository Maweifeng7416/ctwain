#include "stdafx.h"
#include "twain_func.h"


HMODULE TwainFunc::dsm_module_ = nullptr;
DSMENTRYPROC TwainFunc::dsm_entry_ = nullptr;
TW_ENTRYPOINT TwainFunc::memory_entry_{ 0 };

bool TwainFunc::InitializeDSM(){
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


void TwainFunc::UninitializeDSM() {
	dsm_entry_ = nullptr;
	if (dsm_module_) {
		UNLOADLIBRARY(dsm_module_);
		dsm_module_ = nullptr;
	}
}

TW_UINT16 TwainFunc::DSM_Entry(pTW_IDENTITY orig, pTW_IDENTITY dest, TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData) {
	if (dsm_entry_) {
		return dsm_entry_(orig, dest, DG, DAT, MSG, pData);
	}
	return TWRC_FAILURE;
}

void TwainFunc::UpdateMemoryEntry(pTW_IDENTITY appId){
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
TW_HANDLE TwainFunc::Alloc(TW_UINT32 size){
	if (memory_entry_.DSM_MemAllocate){
		return memory_entry_.DSM_MemAllocate(size);
	}
#ifdef TWH_CMP_MSC
	return GlobalAlloc(GPTR, size);
#else
	return malloc(size);
#endif
}

void TwainFunc::Free(TW_HANDLE handle){
	if (memory_entry_.DSM_MemFree){
		memory_entry_.DSM_MemFree(handle);
	}
#ifdef TWH_CMP_MSC
	GlobalFree(handle);
#else
	free(handle);
#endif
}

TW_MEMREF TwainFunc::Lock(TW_HANDLE handle){
	if (memory_entry_.DSM_MemLock){
		return memory_entry_.DSM_MemLock(handle);
	}
#ifdef TWH_CMP_MSC
	return GlobalLock(handle);
#else
	return handle;
#endif
}

void TwainFunc::Unlock(TW_HANDLE handle){
	if (memory_entry_.DSM_MemUnlock){
		memory_entry_.DSM_MemUnlock(handle);
	}
#ifdef TWH_CMP_MSC
	GlobalUnlock(handle);
#endif
}