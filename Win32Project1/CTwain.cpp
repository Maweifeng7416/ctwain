#include "stdafx.h"
#include "CTwain.h"
#include <iostream>

CTwain::CTwain()
{
	_dsmModule = LoadLibrary(L"twaindsm.dll");
	if (_dsmModule)
	{
		_dsmEntry = (DSMENTRYPROC) GetProcAddress(_dsmModule, "DSM_Entry");
		if (_dsmEntry){
			_state = 2;
			FillAppId();
		}
	}
}

CTwain::~CTwain()
{
	ForceStepDown(2);
	if (_dsmModule)
	{
		FreeLibrary(_dsmModule);
	}
}

void CTwain::ForceStepDown(int state){

	if (_state == 7 && _state > state)
	{
		TW_PENDINGXFERS xfer;
		DsmEntry(DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &xfer);
		_state = 6;
	}
	if (_state == 6 && _state > state)
	{
		TW_PENDINGXFERS xfer;
		DsmEntry(DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &xfer);
		_state = 5;
	}
	if (_state == 5 && _state > state)
	{
		DsmEntry(DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &_ui);
		_state = 4;
	}
	if (_state == 4 && _state > state)
	{
		CloseSource();
		_state = 3;
	}
	if (_state == 3 && _state > state)
	{
		CloseDsm();
		_state = 2;
	}
}

void CTwain::OpenDsm(HWND handle)
{
	if (_state == 2)
	{
		_handle = handle;
		TW_UINT16 rc = DsmEntry(DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &_handle);
		if (rc == TWRC_SUCCESS)
		{
			_state = 3;
		}
	}
}
void CTwain::CloseDsm()
{
	if (_state == 3)
	{
		TW_UINT16 rc = DsmEntry(DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &_handle);
		if (rc == TWRC_SUCCESS)
		{
			_state = 2;
		}
	}
}

void CTwain::FillAppId()
{
	_appId.Id = 0;
	_appId.Version.MajorNum = 1;
	_appId.Version.MinorNum = 0;
	_appId.Version.Language = TWLG_ENGLISH_USA;
	_appId.Version.Country = TWCY_USA;
	strncpy_s(_appId.Version.Info, sizeof(_appId.Version.Info), "1.0.0", _TRUNCATE);
	_appId.ProtocolMajor = TWON_PROTOCOLMAJOR;
	_appId.ProtocolMinor = TWON_PROTOCOLMINOR;
	_appId.SupportedGroups = DF_APP2 | DG_IMAGE | DG_CONTROL;
	strncpy_s(_appId.Manufacturer, sizeof(_appId.Manufacturer), "App's Manufacturer", _TRUNCATE);
	strncpy_s(_appId.ProductFamily, sizeof(_appId.ProductFamily), "App's Product Family", _TRUNCATE);
	strncpy_s(_appId.ProductName, sizeof(_appId.ProductName), "Specific App Product Name", _TRUNCATE);
}

TW_IDENTITY CTwain::ShowSelectSourceUI(){
	TW_IDENTITY src;
	if (_state > 2 && _state <= 7)
	{
		auto twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &src);
	}
	return src;
}
TW_UINT16 CTwain::OpenSource(TW_IDENTITY srcId)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state > 2 && _state < 5)
	{
		if (_state == 4)
		{
			CloseSource();
		}

		_srcId = srcId;
		twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &_srcId);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 4;
		}
	}
	return twRC;
}
TW_UINT16 CTwain::CloseSource()
{
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state == 4)
	{
		twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &_srcId);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 3;
		}
	}
	return twRC;
}
TW_UINT16 CTwain::EnableSource(bool modal, bool showUI)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state == 4)
	{
		_ui.hParent = _handle;
		_ui.ModalUI = modal ? TRUE : FALSE;
		_ui.ShowUI = showUI ? TRUE : FALSE;
		twRC = DsmEntry(DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, &_ui);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 5;
		}
	}
	return twRC;
}
TW_UINT16 CTwain::EnableSourceUIOnly(bool modal)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state == 4)
	{
		_ui.hParent = _handle;
		_ui.ModalUI = modal ? TRUE : FALSE;
		_ui.ShowUI = TRUE;
		twRC = DsmEntry(DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDSUIONLY, &_ui);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 5;
		}
	}
	return twRC;
}
bool CTwain::IsTwainMessage(const MSG* msg)
{
	if (_state > 3 && _dsmEntry)
	{
		TW_EVENT evt;
		evt.pEvent = (TW_MEMREF) msg;
		TW_UINT16 twRC = DsmEntry(DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, &evt);
		if (twRC == TWRC_DSEVENT)
		{
			switch (evt.TWMessage){
			case MSG_XFERREADY:
				_state = 6;
				break;
			case MSG_CLOSEDSREQ:
			case MSG_CLOSEDSOK:
				twRC = DsmEntry(DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &_ui);
				if (twRC == TWRC_SUCCESS)
				{
					_state = 4;
				}
				break;
			case MSG_DEVICEEVENT:
				break;
			case MSG_NULL:
				break;
			default:
				//cerr << "\nError - Unknown message in MSG_PROCESSEVENT loop\n" << endl;
				break;
			}
			return true;
		}
	}
	return false;
}

TW_UINT16 CTwain::DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData)
{
	if (_dsmEntry)
	{
		return _dsmEntry(&_appId, &_srcId, DG, DAT, MSG, pData);
	}
	return TWRC_FAILURE;
}