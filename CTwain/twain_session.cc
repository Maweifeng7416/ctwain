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
#include "twain_session.h"
#include "twain_func.h"

namespace ctwain{

	///////////////////////////////////////////////////////
	// private impl
	///////////////////////////////////////////////////////

	class TwainSessionImpl{
	public:
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

	struct RunHandleDsmMsgHack{
		TwainSessionImpl* Instance;
		TW_UINT16 Msg;
		operator unsigned short() const { return TWRC_SUCCESS; }
		~RunHandleDsmMsgHack(){
			Instance->HandleDsmMessage(Msg);
		}
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
					RunHandleDsmMsgHack hack{ Instance, msg };
					//inst.HandleDsmMessage(msg);
					return hack;
				}
			}
			return TWRC_FAILURE;
		}

	};

	TwainSessionImpl* CallbackHack::Instance = nullptr;


	TwainSessionImpl::~TwainSessionImpl(){
		TwainFunc::UninitializeDSM();
	}
	void TwainSessionImpl::Init(){
		if (TwainFunc::InitializeDSM()){
			_state = 2;
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
			TransferReadyEventArgs preXferArgs{ 0 };
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
			//TW_DEVICEEVENT de{ 0 };
			//if (DsmEntry(true, DG_CONTROL, DAT_DEVICEEVENT, MSG_GET, &de) == TWRC_SUCCESS)
			//{
			//	//DeviceEvent(de);
			//}
			//break;
		case MSG_NULL:
			break;
		default:
			std::cerr << "Error - Unknown DSM message " << msg << "." << std::endl;
			break;
		}
	}
	TW_UINT16 TwainSessionImpl::DsmEntry(bool includeSource, TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData)
	{
		return includeSource ?
			TwainFunc::DSM_Entry(&_appId, &_srcId, DG, DAT, MSG, pData) :
			TwainFunc::DSM_Entry(&_appId, nullptr, DG, DAT, MSG, pData);
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
				TwainFunc::UpdateMemoryEntry(&_appId);
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
				TwainFunc::UpdateMemoryEntry(nullptr);
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

	TwainSession::TwainSession() :pimpl_(new TwainSessionImpl())
	{
		/*auto sz1 = sizeof(TwainSession);
		auto sz2 = sizeof(TwainSessionImpl);*/
	}
	TwainSession::TwainSession(const TwainSession& other)
		: pimpl_(new TwainSessionImpl(*other.pimpl_)) {
	}

	TwainSession::TwainSession(TwainSession&& other)
		: pimpl_(0)
	{
		std::swap(pimpl_, other.pimpl_);
	}

	TwainSession& TwainSession::operator=(const TwainSession &other) {
		if (this != &other) {
			*pimpl_ = *(other.pimpl_);
		}
		return *this;
	}
	TwainSession::~TwainSession()
	{
		delete pimpl_;
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

	int TwainSession::GetState(){ return pimpl_->_state; }
	bool TwainSession::IsDsmInitialized(){ return pimpl_->_state > 1; }
	bool TwainSession::IsDsmOpen(){ return pimpl_->_state > 2; }
	bool TwainSession::IsSourceOpen(){ return pimpl_->_state > 3; }
	bool TwainSession::IsSourceEnabled(){ return pimpl_->_state > 4; }
	bool TwainSession::IsTransferring(){ return pimpl_->_state > 5; }

	void TwainSession::ForceStepDown(int state){ pimpl_->ForceStepDown(state); }

	bool TwainSession::Initialize()
	{
		FillAppId(pimpl_->_appId);
		pimpl_->Init();
		return IsDsmInitialized();
	}

	void TwainSession::OpenDsm(TW_MEMREF hWnd) { pimpl_->OpenDsm(hWnd); }
	void TwainSession::CloseDsm(){ pimpl_->CloseDsm(); }

	TW_STATUS TwainSession::GetDsmStatus(){ return pimpl_->GetDsmStatus(); }
	TW_STATUS TwainSession::GetSourceStatus(){ return pimpl_->GetSourceStatus(); }

	std::unique_ptr<TW_IDENTITY> TwainSession::ShowSourceSelector(){ return pimpl_->ShowSourceSelector(); }

	std::unique_ptr<TW_IDENTITY> TwainSession::GetDefaultSource(){ return pimpl_->GetDefaultSource(); }
	std::vector<std::unique_ptr<TW_IDENTITY>> TwainSession::GetSources(){ return pimpl_->GetSources(); }

	TW_UINT16 TwainSession::OpenSource(TW_IDENTITY& source){ return pimpl_->OpenSource(source); }
	TW_UINT16 TwainSession::CloseSource(){ return pimpl_->CloseSource(); }
	TW_UINT16 TwainSession::EnableSource(bool modal, bool showUI){ return pimpl_->EnableSource(false, showUI, modal); }
	TW_UINT16 TwainSession::EnableSourceUIOnly(bool modal){ return pimpl_->EnableSource(true, true, modal); }

	bool TwainSession::IsTwainMessage(const MSG& msg)
	{
		if (pimpl_->_state > 4)
		{
			TW_EVENT evt{ const_cast<MSG*>(&msg) };
			TW_UINT16 twRC = pimpl_->DsmEntry(true, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, &evt);
			if (twRC == TWRC_DSEVENT)
			{
				pimpl_->HandleDsmMessage(evt.TWMessage);
				return true;
			}
		}
		return false;
	}

	TW_UINT16 TwainSession::DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData){ return pimpl_->DsmEntry(true, DG, DAT, MSG, pData); }
}