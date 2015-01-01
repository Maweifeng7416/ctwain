#pragma once
#include "twain2.3.h"
class CTwain
{
private:
	HMODULE _dsmModule = nullptr;
	TW_IDENTITY _appId;
	TW_IDENTITY _srcId;
	DSMENTRYPROC _dsmEntry = nullptr;
	HWND _handle = nullptr;
	TW_USERINTERFACE _ui;
	int _state = 1;
	void FillAppId();
	void ForceStepDown(int state);
public:
	CTwain();
	~CTwain();
	int GetState(){ return _state; }
	void OpenDsm(HWND handle);
	void CloseDsm();
	TW_STATUS GetDsmStatus();
	TW_STATUS GetSourceStatus();
	TW_IDENTITY ShowSelectSourceUI();
	TW_UINT16 OpenSource(TW_IDENTITY srcId);
	TW_UINT16 CloseSource();
	TW_UINT16 EnableSource(bool modal, bool showUI);
	TW_UINT16 EnableSourceUIOnly(bool modal);

	bool IsTwainMessage(const MSG* message);
	TW_UINT16 DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
};

