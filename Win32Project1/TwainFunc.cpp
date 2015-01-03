#include "stdafx.h"
#include "TwainFunc.h"


HMODULE TwainFunc::_dsmModule = nullptr;
DSMENTRYPROC TwainFunc::_dsmEntry = nullptr;
TW_ENTRYPOINT TwainFunc::_memEntry{ 0 };

bool TwainFunc::InitializeDSM(){
	if (!_dsmModule){

#ifdef TWH_CMP_MSC
		_dsmModule = LOADLIBRARY(L"twaindsm.dll");
#elif
		_dsmModule = LOADLIBRARY("/usr/local/lib/libtwaindsm.so");
#endif
		if (_dsmModule){
			_dsmEntry = (DSMENTRYPROC) LOADFUNCTION(_dsmModule, "DSM_Entry");
			if (_dsmEntry){
				return true;
			}
			else{
				FreeLibrary(_dsmModule);
				_dsmModule = nullptr;
			}
		}
	}
	return _dsmEntry;
}


void TwainFunc::UninitializeDSM() {
	_dsmEntry = nullptr;
	if (_dsmModule) {
		UNLOADLIBRARY(_dsmModule);
		_dsmModule = nullptr;
	}
}

TW_UINT16 TwainFunc::DSM_Entry(pTW_IDENTITY orig, pTW_IDENTITY dest, TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData) {
	if (_dsmEntry) {
		return _dsmEntry(orig, dest, DG, DAT, MSG, pData);
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
				_memEntry = ep;
				return;
			}
		}
	}
	_memEntry = { 0 };
}
TW_HANDLE TwainFunc::Alloc(TW_UINT32 size){
	if (_memEntry.DSM_MemAllocate){
		return _memEntry.DSM_MemAllocate(size);
	}
#ifdef TWH_CMP_MSC
	return GlobalAlloc(GPTR, size);
#endif
	return nullptr;
}

void TwainFunc::Free(TW_HANDLE handle){
	if (_memEntry.DSM_MemFree){
		_memEntry.DSM_MemFree(handle);
	}
#ifdef TWH_CMP_MSC
	GlobalFree(handle);
#endif
}

TW_MEMREF TwainFunc::Lock(TW_HANDLE handle){
	if (_memEntry.DSM_MemLock){
		return _memEntry.DSM_MemLock(handle);
	}
#ifdef TWH_CMP_MSC
	return GlobalLock(handle);
#endif
	return nullptr;
}

void TwainFunc::Unlock(TW_HANDLE handle){
	if (_memEntry.DSM_MemUnlock){
		_memEntry.DSM_MemUnlock(handle);
	}
#ifdef TWH_CMP_MSC
	GlobalUnlock(handle);
#endif
}