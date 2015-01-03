#include "stdafx.h"
#include "TwainSession.h"
#include <iostream>

// private

void TwainSession::FillAppId()
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
void TwainSession::DisableSource()
{
	auto twRC = DsmEntry(DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &_ui);
	if (twRC == TWRC_SUCCESS)
	{
		_state = 4;
	}
	
}
void TwainSession::DoTransfer()
{
	TW_PENDINGXFERS pending;
	TW_UINT16 rc = TWRC_SUCCESS;

	do
	{
		TransferReadyEventArgs preXferArgs;
		memset(&preXferArgs, 0, sizeof(preXferArgs));
		TransferReady(preXferArgs);
		if (preXferArgs.CancelAll)
		{
			rc = DsmEntry(DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pending);
		}
		else
		{
			if (!preXferArgs.CancelCurrent)
			{

			}
			rc = DsmEntry(DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &pending);
		}

	} while (rc == TWRC_SUCCESS && pending.Count != 0);

	// some poorly written scanner drivers return failure on EndXfer so also check for pending count now.
	// this may break with other sources but we'll see
	if (pending.Count == 0 && _state > 5)
	{
		_state = 5;
		DisableSource();
	}
}
TW_UINT16 TW_CALLINGSTYLE TwainSession::DsmCallback(pTW_IDENTITY pOrigin, pTW_IDENTITY pDest, TW_UINT32    DG, TW_UINT16    DAT, TW_UINT16    MSG, TW_MEMREF    pData){

	if (pOrigin && pOrigin->Id == _srcId.Id){
		HandleDsmMessage(MSG);
		return TWRC_SUCCESS;
	}
	return TWRC_FAILURE;
}
void TwainSession::HandleDsmMessage(TW_UINT16 msg){

	switch (msg){
	case MSG_XFERREADY:
		_state = 6;
		DoTransfer();
		break;
	case MSG_CLOSEDSREQ:
	case MSG_CLOSEDSOK:
		ForceStepDown(5);
		DisableSource();
		break;
	case MSG_DEVICEEVENT:
		TW_DEVICEEVENT de;
		memset(&de, 0, sizeof(de));
		if (DsmEntry(DG_CONTROL, DAT_DEVICEEVENT, MSG_GET, &de) == TWRC_SUCCESS)
		{
			DeviceEvent(de);
		}
		break;
	case MSG_NULL:
		break;
	default:
		std::cerr << "Error - Unknown DSM message " << msg << "." << std::endl;
		break;
	}
}

// public

TwainSession::TwainSession()
{
	FillAppId();
}
TwainSession::~TwainSession()
{
	_dsmEntry = nullptr;
	if (_dsmModule)
	{
		FreeLibrary(_dsmModule);
		_dsmModule = nullptr;
	}
}
void TwainSession::ForceStepDown(int state){

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
		DisableSource();
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

bool TwainSession::Initialize()
{
	_dsmModule = LoadLibrary(L"twaindsm.dll");
	if (_dsmModule)
	{
		_dsmEntry = (DSMENTRYPROC) GetProcAddress(_dsmModule, "DSM_Entry");
		if (_dsmEntry){
			_state = 2;
		}
		else{
			FreeLibrary(_dsmModule);
			_dsmModule = nullptr;
		}
	}
	return IsDsmInitialized();
}

void TwainSession::OpenDsm(HWND handle)
{
	if (_state == 2)
	{
		_handle = handle;
		TW_UINT16 rc = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &_handle);
		if (rc == TWRC_SUCCESS)
		{
			_state = 3;
		}
	}
}
void TwainSession::CloseDsm()
{
	if (_state == 3)
	{
		TW_UINT16 rc = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &_handle);
		if (rc == TWRC_SUCCESS)
		{
			_state = 2;
		}
	}
}

TW_STATUS TwainSession::GetDsmStatus()
{
	TW_STATUS status;
	if (_state > 1)
	{
		_dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_STATUS, MSG_GET, &status);
	}
	return status;
}
TW_STATUS TwainSession::GetSourceStatus()
{
	TW_STATUS status;
	if (_state > 3)
	{
		DsmEntry(DG_CONTROL, DAT_STATUS, MSG_GET, &status);
	}
	return status;
}

std::unique_ptr<TW_IDENTITY> TwainSession::ShowSourceSelector()
{
	if (_state > 2)
	{
		auto src = std::make_unique<TW_IDENTITY>();
		auto twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, src.get());
		if (twRC == TWRC_SUCCESS){
			return src;
		}
	}
	return nullptr;
}

std::unique_ptr<TW_IDENTITY> TwainSession::GetDefaultSource()
{
	if (_state > 2)
	{
		auto src = std::make_unique<TW_IDENTITY>();
		auto twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, src.get());
		if (twRC == TWRC_SUCCESS){
			return src;
		}
	}
	return nullptr;
}
std::vector<std::unique_ptr<TW_IDENTITY>> TwainSession::GetSources(){
	std::vector<std::unique_ptr<TW_IDENTITY>> list;
	if (_state > 2){
		auto src = std::make_unique<TW_IDENTITY>();
		auto twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_GETFIRST, src.get());
		while (twRC == TWRC_SUCCESS){
			list.push_back(std::move(src));

			src = std::make_unique<TW_IDENTITY>();
			twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_GETNEXT, src.get());
		}
	}
	return list;
}

TW_UINT16 TwainSession::OpenSource(TW_IDENTITY& source)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state > 2 && _state < 5)
	{
		if (_state == 4)
		{
			CloseSource();
		}

		twRC = _dsmEntry(&_appId, nullptr, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &source);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 4;
			_srcId = source;
		}
	}
	return twRC;
}
TW_UINT16 TwainSession::CloseSource()
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
TW_UINT16 TwainSession::EnableSource(bool modal, bool showUI)
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
TW_UINT16 TwainSession::EnableSourceUIOnly(bool modal)
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

bool TwainSession::IsTwainMessage(const MSG& msg)
{
	if (_state > 4)
	{
		TW_EVENT evt{ const_cast<MSG*>(&msg) };
		TW_UINT16 twRC = DsmEntry(DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, &evt);
		if (twRC == TWRC_DSEVENT)
		{
			HandleDsmMessage(evt.TWMessage);
			return true;
		}
	}
	return false;
}
TW_UINT16 TwainSession::DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData)
{
	if (_dsmEntry)
	{
		return _dsmEntry(&_appId, &_srcId, DG, DAT, MSG, pData);
	}
	return TWRC_FAILURE;
}