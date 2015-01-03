#include "stdafx.h"
#include "TwainSession.h"
#include <iostream>

///////////////////////////////////////////////////////
// private impl
///////////////////////////////////////////////////////

struct TwainSessionImpl{

	HMODULE _dsmModule = nullptr;
	DSMENTRYPROC _dsmEntry = nullptr;
	TW_MEMREF _hWnd = nullptr;
	int _state = 1;

	TW_USERINTERFACE _ui;
	TW_IDENTITY _appId;
	TW_IDENTITY _srcId;

	~TwainSessionImpl();
	void Init();
	void OpenDsm(TW_MEMREF hWnd = nullptr);
	void CloseDsm();

	TW_STATUS GetDsmStatus();
	TW_STATUS GetSourceStatus();
	std::unique_ptr<TW_IDENTITY> ShowSourceSelector();
	std::unique_ptr<TW_IDENTITY> GetDefaultSource();
	std::vector<std::unique_ptr<TW_IDENTITY>> GetSources();
	void TryRegisterCallback();
	TW_UINT16 OpenSource(TW_IDENTITY& source);
	TW_UINT16 CloseSource();
	TW_UINT16 EnableSource(bool uiOnly, bool showUI, bool modal);
	void DisableSource();
	void DoTransfer();

	void HandleDsmMessage(TW_UINT16);
	TW_UINT16 DsmEntry(bool includeSource, TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);
	void ForceStepDown(int state);
};

// hack to use static func for callback with current session instance
struct CallbackHack{

	static TwainSessionImpl* Instance;
	static
#ifdef TWH_CMP_MSC
		TW_UINT16 FAR PASCAL
#else
		FAR PASCAL TW_UINT16
#endif
		DsmCallback(pTW_IDENTITY orig, pTW_IDENTITY dest, TW_UINT32 dg, TW_UINT16 dat, TW_UINT16 msg, TW_MEMREF data){
		if (Instance){
			auto inst = *Instance;
			if (orig && orig->Id == inst._srcId.Id){
				inst.HandleDsmMessage(msg);
				return TWRC_SUCCESS;
			}
		}
		return TWRC_FAILURE;
	}

};

TwainSessionImpl* CallbackHack::Instance = nullptr;


TwainSessionImpl::~TwainSessionImpl(){
	_dsmEntry = nullptr;
	if (_dsmModule)
	{
		FreeLibrary(_dsmModule);
		_dsmModule = nullptr;
	}
}
void TwainSessionImpl::Init(){
	if (!_dsmModule){
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
	}
}
void TwainSessionImpl::TryRegisterCallback(){

	return;//doesn't work yet

	auto funcPtr = CallbackHack::DsmCallback;
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_appId.ProtocolMajor > 2 || (_appId.ProtocolMajor >= 2 && _appId.ProtocolMinor >= 3)){
		// callback2
		TW_CALLBACK2 callback{ funcPtr, 0, 0 };
		twRC = DsmEntry(true, DG_CONTROL, DAT_CALLBACK2, MSG_REGISTER_CALLBACK, &callback);
	}
	else{
		// old callback
		TW_CALLBACK callback{ funcPtr, 0, 0 };
		twRC = DsmEntry(true, DG_CONTROL, DAT_CALLBACK, MSG_REGISTER_CALLBACK, &callback);
	}
	if (twRC == TWRC_SUCCESS){
		CallbackHack::Instance = this;
	}
	else{
		CallbackHack::Instance = nullptr;
	}
}

TW_UINT16 TwainSessionImpl::EnableSource(bool uiOnly, bool showUI, bool modal){

	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state == 4)
	{
		_ui.hParent = _hWnd;
		_ui.ModalUI = modal ? TRUE : FALSE;
		_ui.ShowUI = showUI ? TRUE : FALSE;
		twRC = uiOnly ?
			DsmEntry(true, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDSUIONLY, &_ui) :
			DsmEntry(true, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, &_ui);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 5;
		}
	}
	return twRC;
}
void TwainSessionImpl::DisableSource()
{
	auto twRC = DsmEntry(true, DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &_ui);
	if (twRC == TWRC_SUCCESS)
	{
		_state = 4;
	}

}
void TwainSessionImpl::DoTransfer()
{
	TW_PENDINGXFERS pending;
	TW_UINT16 rc = TWRC_SUCCESS;

	do
	{
		TransferReadyEventArgs preXferArgs;
		memset(&preXferArgs, 0, sizeof(preXferArgs));
		//TransferReady(preXferArgs);
		if (preXferArgs.CancelAll)
		{
			rc = DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pending);
		}
		else
		{
			if (!preXferArgs.CancelCurrent)
			{

			}
			rc = DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &pending);
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
void TwainSessionImpl::HandleDsmMessage(TW_UINT16 msg){

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
		if (DsmEntry(true, DG_CONTROL, DAT_DEVICEEVENT, MSG_GET, &de) == TWRC_SUCCESS)
		{
			//DeviceEvent(de);
		}
		break;
	case MSG_NULL:
		break;
	default:
		std::cerr << "Error - Unknown DSM message " << msg << "." << std::endl;
		break;
	}
}
TW_UINT16 TwainSessionImpl::DsmEntry(bool includeSource, TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData)
{
	if (_state > 1)
	{
		return includeSource ?
			_dsmEntry(&_appId, &_srcId, DG, DAT, MSG, pData) :
			_dsmEntry(&_appId, nullptr, DG, DAT, MSG, pData);
	}
	return TWRC_FAILURE;
}

void TwainSessionImpl::ForceStepDown(int state){

	if (_state == 7 && _state > state)
	{
		TW_PENDINGXFERS xfer;
		DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &xfer);
		_state = 6;
	}
	if (_state == 6 && _state > state)
	{
		TW_PENDINGXFERS xfer;
		DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &xfer);
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

void TwainSessionImpl::OpenDsm(TW_MEMREF hWnd)
{
	if (_state == 2)
	{
		_hWnd = hWnd;
		TW_UINT16 rc = DsmEntry(false, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &_hWnd);
		if (rc == TWRC_SUCCESS)
		{
			_state = 3;
		}
	}
}
void TwainSessionImpl::CloseDsm()
{
	if (_state == 3)
	{
		TW_UINT16 rc = DsmEntry(false, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &_hWnd);
		if (rc == TWRC_SUCCESS)
		{
			_state = 2;
		}
	}
}

TW_STATUS TwainSessionImpl::GetDsmStatus()
{
	TW_STATUS status;
	if (_state > 1)
	{
		DsmEntry(false, DG_CONTROL, DAT_STATUS, MSG_GET, &status);
	}
	return status;
}
TW_STATUS TwainSessionImpl::GetSourceStatus()
{
	TW_STATUS status;
	if (_state > 3)
	{
		DsmEntry(true, DG_CONTROL, DAT_STATUS, MSG_GET, &status);
	}
	return status;
}

std::unique_ptr<TW_IDENTITY> TwainSessionImpl::ShowSourceSelector()
{
	if (_state > 2)
	{
		auto src = std::make_unique<TW_IDENTITY>();
		auto twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, src.get());
		if (twRC == TWRC_SUCCESS){
			return src;
		}
	}
	return nullptr;
}

std::unique_ptr<TW_IDENTITY> TwainSessionImpl::GetDefaultSource()
{
	if (_state > 2)
	{
		auto src = std::make_unique<TW_IDENTITY>();
		auto twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, src.get());
		if (twRC == TWRC_SUCCESS){
			return src;
		}
	}
	return nullptr;
}
std::vector<std::unique_ptr<TW_IDENTITY>> TwainSessionImpl::GetSources(){
	std::vector<std::unique_ptr<TW_IDENTITY>> list;
	if (_state > 2){
		auto src = std::make_unique<TW_IDENTITY>();
		auto twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_GETFIRST, src.get());
		while (twRC == TWRC_SUCCESS){
			list.push_back(std::move(src));

			src = std::make_unique<TW_IDENTITY>();
			twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_GETNEXT, src.get());
		}
	}
	return list;
}

TW_UINT16 TwainSessionImpl::OpenSource(TW_IDENTITY& source)
{
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state > 2 && _state < 5)
	{
		if (_state == 4)
		{
			CloseSource();
		}

		twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &source);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 4;
			_srcId = source;
			TryRegisterCallback();
		}
	}
	return twRC;
}
TW_UINT16 TwainSessionImpl::CloseSource()
{
	TW_UINT16 twRC = TWRC_FAILURE;
	if (_state == 4)
	{
		twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &_srcId);
		if (twRC == TWRC_SUCCESS)
		{
			_state = 3;
			CallbackHack::Instance = nullptr;
		}
	}
	return twRC;
}


///////////////////////////////////////////////////////
// public
///////////////////////////////////////////////////////

TwainSession::TwainSession() :_pimpl(new TwainSessionImpl())
{
	auto sz1 = sizeof(TwainSession);
	auto sz2 = sizeof(TwainSessionImpl);
}
TwainSession::TwainSession(const TwainSession& other)
	: _pimpl(new TwainSessionImpl(*other._pimpl)) {
}

TwainSession::TwainSession(TwainSession&& other)
	: _pimpl(0)
{
	std::swap(_pimpl, other._pimpl);
}

TwainSession& TwainSession::operator=(const TwainSession &other) {
	if (this != &other) {
		*_pimpl = *(other._pimpl);
	}
	return *this;
}
TwainSession::~TwainSession()
{
	delete _pimpl;
}
void TwainSession::FillAppId(TW_IDENTITY& appId)
{
	appId.Id = 0;
	appId.ProtocolMajor = TWON_PROTOCOLMAJOR;
	appId.ProtocolMinor = TWON_PROTOCOLMINOR;
	appId.SupportedGroups = DF_APP2 | DG_IMAGE | DG_CONTROL;
	appId.Version.MajorNum = 1;
	appId.Version.MinorNum = 0;
	appId.Version.Language = TWLG_ENGLISH_USA;
	appId.Version.Country = TWCY_USA;
	strncpy_s(appId.Version.Info, sizeof(appId.Version.Info), "1.0.0", _TRUNCATE);
	strncpy_s(appId.Manufacturer, sizeof(appId.Manufacturer), "App's Manufacturer", _TRUNCATE);
	strncpy_s(appId.ProductFamily, sizeof(appId.ProductFamily), "App's Product Family", _TRUNCATE);
	strncpy_s(appId.ProductName, sizeof(appId.ProductName), "Specific App Product Name", _TRUNCATE);
}

int TwainSession::GetState(){ return _pimpl->_state; }
bool TwainSession::IsDsmInitialized(){ return _pimpl->_state > 1; }
bool TwainSession::IsDsmOpen(){ return _pimpl->_state > 2; }
bool TwainSession::IsSourceOpen(){ return _pimpl->_state > 3; }
bool TwainSession::IsSourceEnabled(){ return _pimpl->_state > 4; }
bool TwainSession::IsTransferring(){ return _pimpl->_state > 5; }

void TwainSession::ForceStepDown(int state){

	_pimpl->ForceStepDown(state);
}

bool TwainSession::Initialize()
{
	FillAppId(_pimpl->_appId);
	_pimpl->Init();
	return IsDsmInitialized();
}

void TwainSession::OpenDsm(TW_MEMREF hWnd)
{
	_pimpl->OpenDsm(hWnd);
}
void TwainSession::CloseDsm()
{
	_pimpl->CloseDsm();
}

TW_STATUS TwainSession::GetDsmStatus()
{
	return _pimpl->GetDsmStatus();
}
TW_STATUS TwainSession::GetSourceStatus()
{
	return _pimpl->GetSourceStatus();
}

std::unique_ptr<TW_IDENTITY> TwainSession::ShowSourceSelector()
{
	return _pimpl->ShowSourceSelector();
}

std::unique_ptr<TW_IDENTITY> TwainSession::GetDefaultSource()
{
	return _pimpl->GetDefaultSource();
}
std::vector<std::unique_ptr<TW_IDENTITY>> TwainSession::GetSources(){
	return _pimpl->GetSources();
}

TW_UINT16 TwainSession::OpenSource(TW_IDENTITY& source)
{
	return _pimpl->OpenSource(source);
}
TW_UINT16 TwainSession::CloseSource()
{
	return _pimpl->CloseSource();
}
TW_UINT16 TwainSession::EnableSource(bool modal, bool showUI)
{
	return _pimpl->EnableSource(false, showUI, modal);
}
TW_UINT16 TwainSession::EnableSourceUIOnly(bool modal)
{
	return _pimpl->EnableSource(true, true, modal);
}

bool TwainSession::IsTwainMessage(const MSG& msg)
{
	if (_pimpl->_state > 4)
	{
		TW_EVENT evt{ const_cast<MSG*>(&msg) };
		TW_UINT16 twRC = DsmEntry(DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, &evt);
		if (twRC == TWRC_DSEVENT)
		{
			_pimpl->HandleDsmMessage(evt.TWMessage);
			return true;
		}
	}
	return false;
}

TW_UINT16 TwainSession::DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData)
{
	return _pimpl->DsmEntry(true, DG, DAT, MSG, pData);
}