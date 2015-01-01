#pragma once
#include "twain2.3.h"
class CTwain
{
private:
	HMODULE _dsmModule = nullptr;
	TW_IDENTITY _appId;
	pTW_IDENTITY _srcId = nullptr;
	DSMENTRYPROC _dsmEntry = nullptr;
	HWND _handle = nullptr;
	int _state = 1;
	void FillAppId();
public:
	CTwain();
	~CTwain();
	int GetState(){ return _state; }
	void OpenDsm(HWND handle);
	void CloseDsm();
	pTW_IDENTITY ShowSelectSourceUI();
	TW_UINT16 OpenSource(pTW_IDENTITY srcId);
	TW_UINT16 CloseSource();
	TW_UINT16 EnableSource(bool modal, bool showUI);

	bool HandleTwainMessage(const MSG* message);
	TW_UINT16 DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
};

