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



	///////////////////////////////////////////////////////
	// tests
	///////////////////////////////////////////////////////


	// temp hack to use static func for callback with current session instance
	// this doesn't work yet.
	struct CallbackHack{

		static TwainSession* Instance;
		static
#ifdef TWH_CMP_MSC
			TW_UINT16 FAR PASCAL
#else
			FAR PASCAL TW_UINT16
#endif
			DsmCallback(pTW_IDENTITY orig, pTW_IDENTITY, TW_UINT32, TW_UINT16, TW_UINT16 /*msg*/, TW_MEMREF){
			if (Instance){
				if (orig && orig->Id == Instance->source_id()){
					//Instance->HandleDsmMessage(msg);
					return TWRC_SUCCESS;
				}
			}
			return TWRC_FAILURE;
		}

	};

	TwainSession* CallbackHack::Instance = nullptr;




	///////////////////////////////////////////////////////
	// public twain session parts
	///////////////////////////////////////////////////////



	TwainSession::~TwainSession(){
		EntryPoints::UninitializeDSM();

		if (loop_){
			delete loop_;
			loop_ = nullptr;
		}
	}

	TW_UINT32 TwainSession::source_id(){
		return ds_id_.Id;
	}

	bool TwainSession::Initialize(){
		if (state_ < State::kDsmLoaded && EntryPoints::InitializeDSM()){
			state_ = State::kDsmLoaded;
			OnFillAppId(app_id_);
		}
		return state_ == State::kDsmLoaded;
	}


	void TwainSession::ForceStepDown(State state){

		if (state_ == State::kTransferring && state_ > state)
		{
			TW_PENDINGXFERS xfer;
			DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &xfer);
			state_ = State::kTransferReady;
		}
		if (state_ == State::kTransferReady && state_ > state)
		{
			TW_PENDINGXFERS xfer;
			DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &xfer);
			state_ = State::kSourceEnabled;
		}
		if (state_ == State::kSourceEnabled && state_ > state)
		{
			DisableSource();
			state_ = State::kSourceOpened;
		}
		if (state_ == State::kSourceOpened && state_ > state)
		{
			CloseSource();
			state_ = State::kDsmOpened;
		}
		if (state_ == State::kDsmOpened && state_ > state)
		{
			CloseDsm();
			state_ = State::kDsmLoaded;
		}
	}


	TW_UINT16 TwainSession::OpenDsm(HWND hWnd)
	{
		if (state_ == State::kDsmLoaded)
		{
			if (loop_){
				delete loop_;
				loop_ = nullptr;
			}
			if (hWnd){
				loop_ = new MessageLoop{ hWnd };
			}
			else{
				loop_ = new MessageLoop(this);
			}

			TW_UINT16 rc = DsmEntry(false, DG_CONTROL, DAT_PARENT, MSG_OPENDSM, &hWnd);
			if (rc == TWRC_SUCCESS)
			{
				state_ = State::kDsmOpened;
				EntryPoints::UpdateMemoryEntry(&app_id_);
			}
			return rc;
		}
		return TWRC_FAILURE;
	}
	TW_UINT16 TwainSession::CloseDsm()
	{
		if (state_ == State::kDsmOpened)
		{
			auto handle = loop_->parent_handle();
			TW_UINT16 rc = DsmEntry(false, DG_CONTROL, DAT_PARENT, MSG_CLOSEDSM, &handle);
			if (rc == TWRC_SUCCESS)
			{
				state_ = State::kDsmLoaded;
				EntryPoints::UpdateMemoryEntry(nullptr);
				if (loop_){
					delete loop_;
					loop_ = nullptr;
				}
			}
			return rc;
		}
		return TWRC_FAILURE;
	}

	TW_STATUS TwainSession::GetDsmStatus()
	{
		TW_STATUS status{ 0 };
		if (state_ >= State::kDsmLoaded)
		{
			DsmEntry(false, DG_CONTROL, DAT_STATUS, MSG_GET, &status);
		}
		return status;
	}
	TW_STATUS TwainSession::GetSourceStatus()
	{
		TW_STATUS status{ 0 };
		if (state_ >= State::kSourceOpened)
		{
			DsmEntry(true, DG_CONTROL, DAT_STATUS, MSG_GET, &status);
		}
		return status;
	}

	TW_IDENTITY TwainSession::ShowSourceSelector()
	{
		TW_IDENTITY src{ 0 };
		if (state_ >= State::kDsmOpened)
		{
			DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_USERSELECT, &src);
		}
		return src;
	}
	TW_IDENTITY TwainSession::GetDefaultSource()
	{
		TW_IDENTITY src{ 0 };
		if (state_ >= State::kDsmOpened)
		{
			DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_GETDEFAULT, &src);
		}
		return src;
	}
	std::vector<TW_IDENTITY> TwainSession::GetSources(){
		std::vector<TW_IDENTITY> list;
		if (state_ >= State::kDsmOpened){
			TW_IDENTITY src{ 0 };
			auto twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_GETFIRST, &src);
			while (twRC == TWRC_SUCCESS){
				list.push_back(src);

				twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_GETNEXT, &src);
			}
		}
		return list;
	}

	TW_UINT16 TwainSession::OpenSource(TW_IDENTITY& source)
	{
		TW_UINT16 twRC = TWRC_FAILURE;
		if (state_ >= State::kDsmOpened && state_ < State::kSourceEnabled)
		{
			if (state_ == State::kSourceOpened)
			{
				CloseSource();
			}

			twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_OPENDS, &source);
			if (twRC == TWRC_SUCCESS)
			{
				state_ = State::kSourceOpened;
				ds_id_ = source;
				TryRegisterCallback();
			}
		}
		return twRC;
	}
	TW_UINT16 TwainSession::CloseSource()
	{
		TW_UINT16 twRC = TWRC_FAILURE;
		if (state_ == State::kSourceOpened)
		{
			twRC = DsmEntry(false, DG_CONTROL, DAT_IDENTITY, MSG_CLOSEDS, &ds_id_);
			if (twRC == TWRC_SUCCESS)
			{
				state_ = State::kDsmOpened;
				CallbackHack::Instance = nullptr;
			}
		}
		return twRC;
	}

	TW_UINT16 TwainSession::EnableSource(EnableSourceMode mode, bool modal){

		TW_UINT16 twRC = TWRC_FAILURE;
		if (state_ == State::kSourceOpened)
		{
			ui_.hParent = loop_->parent_handle();
			ui_.ModalUI = modal ? TRUE : FALSE;
			ui_.ShowUI = mode == EnableSourceMode::kHideUI ? FALSE : TRUE;
			twRC = mode == EnableSourceMode::kShowUIOnly ?
				DsmEntry(true, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDSUIONLY, &ui_) :
				DsmEntry(true, DG_CONTROL, DAT_USERINTERFACE, MSG_ENABLEDS, &ui_);
			if (twRC == TWRC_SUCCESS)
			{
				state_ = State::kSourceEnabled;
			}
		}
		return twRC;
	}

	bool TwainSession::IsTwainMessage(const MSG& msg)
	{
		if (state_ >= State::kSourceEnabled)
		{
			TW_EVENT evt{ const_cast<MSG*>(&msg) };
			TW_UINT16 twRC = DsmEntry(true, DG_CONTROL, DAT_EVENT, MSG_PROCESSEVENT, &evt);
			if (twRC == TWRC_DSEVENT)
			{
				std::cout << "Received TWAIN message " << evt.TWMessage << " from loop" << std::endl;
				HandleDsmMessage(evt.TWMessage);
				return true;
			}
		}
		return false;
	}

	TW_UINT16 TwainSession::DsmEntry(bool includeSource, TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData)
	{
		return includeSource ?
			EntryPoints::DSM_Entry(&app_id_, &ds_id_, DG, DAT, MSG, pData) :
			EntryPoints::DSM_Entry(&app_id_, nullptr, DG, DAT, MSG, pData);
	}


	///////////////////////////////////////////////////////
	// "event" methods
	///////////////////////////////////////////////////////


	void TwainSession::OnFillAppId(TW_IDENTITY& appId)
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



	///////////////////////////////////////////////////////
	// private twain session parts
	///////////////////////////////////////////////////////



	void TwainSession::DisableSource()
	{
		auto twRC = DsmEntry(true, DG_CONTROL, DAT_USERINTERFACE, MSG_DISABLEDS, &ui_);
		if (twRC == TWRC_SUCCESS)
		{
			state_ = State::kSourceOpened;
			OnSourceDisabled();
		}

	}
	void TwainSession::TryRegisterCallback(){

		return;//doesn't work yet

		//auto funcPtr = CallbackHack::DsmCallback;
		//TW_UINT16 twRC = TWRC_FAILURE;
		//if (app_id_.ProtocolMajor > 2 || (app_id_.ProtocolMajor >= 2 && app_id_.ProtocolMinor >= 3)){
		//	// callback2
		//	TW_CALLBACK2 callback{ funcPtr, 0, 0 };
		//	twRC = DsmEntry(true, DG_CONTROL, DAT_CALLBACK2, MSG_REGISTER_CALLBACK, &callback);
		//}
		//else{
		//	// old callback
		//	TW_CALLBACK callback{ funcPtr, 0, 0 };
		//	twRC = DsmEntry(true, DG_CONTROL, DAT_CALLBACK, MSG_REGISTER_CALLBACK, &callback);
		//}
		//if (twRC == TWRC_SUCCESS){
		//	CallbackHack::Instance = this;
		//}
		//else{
		//	CallbackHack::Instance = nullptr;
		//}
	}
	void TwainSession::HandleTransferReady()
	{
		TW_PENDINGXFERS pending;
		TW_UINT16 rc = DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_GET, &pending);;

		do
		{
			TransferReadyEventArgs preXferArgs{ 0 };
			preXferArgs.PendingTransferCount = static_cast<TW_INT16>(pending.Count); // good idea? check with spec
			preXferArgs.EndOfJob = pending.EOJ == 0;

			auto xferImage = true;
			auto xferAudio = false;
			TW_UINT32 xferGroup{ DG_IMAGE };
			if (DsmEntry(true, DG_CONTROL, DAT_XFERGROUP, MSG_GET, &xferGroup) == TWRC_SUCCESS){
				xferAudio = (xferGroup & DG_AUDIO) == DG_AUDIO;
				xferImage = xferGroup == 0 || (xferGroup & DG_IMAGE) == DG_IMAGE;
			}

			if (xferImage){
				auto info = std::make_unique<TW_IMAGEINFO>();
				if (DsmEntry(true, DG_IMAGE, DAT_IMAGEINFO, MSG_GET, info.get()) == TWRC_SUCCESS){
					preXferArgs.PendingImageInfo = std::move(info);
				}
			}
			if (xferAudio){
				auto info = std::make_unique<TW_AUDIOINFO>();
				if (DsmEntry(true, DG_IMAGE, DAT_AUDIOINFO, MSG_GET, info.get()) == TWRC_SUCCESS){
					preXferArgs.AudioInfo = std::move(info);
				}
			}

			OnTransferReady(preXferArgs);


			if (preXferArgs.CancelAll)
			{
				rc = DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_RESET, &pending);
			}
			else
			{
				if (!preXferArgs.CancelCurrent)
				{
					TW_UINT16 mech{ TWSX_NATIVE };

					//TODO: need ability to read xfer mech cap value out now

					if (xferImage){
						switch (mech){
						case TWSX_MEMORY:
							break;
						case TWSX_FILE:
							break;
						case TWSX_MEMFILE:
							break;
						case TWSX_NATIVE:
						default:
							TransferNative(true);
							break;
						}
					}
					if (xferAudio){
						switch (mech){
						case TWSX_FILE:
							break;
						case TWSX_NATIVE:
						default:
							TransferNative(false);
							break;
						}
					}
				}
				rc = DsmEntry(true, DG_CONTROL, DAT_PENDINGXFERS, MSG_ENDXFER, &pending);
			}

		} while (rc == TWRC_SUCCESS && pending.Count != 0);

		// some poorly written scanner drivers return failure on EndXfer so also check for pending count now.
		// this may break with other sources but we'll see
		if (pending.Count == 0 && state_ > State::kSourceEnabled)
		{
			state_ = State::kSourceEnabled;
			DisableSource();
		}
	}

	void TwainSession::TransferNative(bool image){
		TW_MEMREF pData = nullptr;

		auto rc = image ?
			DsmEntry(true, DG_IMAGE, DAT_IMAGENATIVEXFER, MSG_GET, &pData) :
			DsmEntry(true, DG_AUDIO, DAT_AUDIONATIVEXFER, MSG_GET, &pData);


		if (rc == TWRC_XFERDONE){
			state_ = State::kTransferring;

			TransferredDataEventArgs tde{ 0 };

			if (image){
				auto info = std::make_unique<TW_IMAGEINFO>();
				if (DsmEntry(true, DG_IMAGE, DAT_IMAGEINFO, MSG_GET, info.get()) == TWRC_SUCCESS){
					tde.ImageInfo = std::move(info);
				}
			}

			tde.NativeData = EntryPoints::Lock(pData);
			OnTransferredData(tde);
			state_ = State::kTransferReady;
			if (tde.NativeData){
				EntryPoints::Unlock(pData);
			}
			if (pData){
				EntryPoints::Free(pData);
			}
		}
		else{
			//auto status = GetSourceStatus();
		}
	}

	void TwainSession::HandleDsmMessage(TW_UINT16 msg){

		switch (msg){
		case MSG_XFERREADY:
			state_ = State::kTransferReady;
			HandleTransferReady();
			break;
		case MSG_CLOSEDSREQ:
		case MSG_CLOSEDSOK:
			ForceStepDown(State::kSourceEnabled);
			DisableSource();
			break;
		case MSG_DEVICEEVENT:
		{
			TW_DEVICEEVENT de{ 0 };
			if (DsmEntry(true, DG_CONTROL, DAT_DEVICEEVENT, MSG_GET, &de) == TWRC_SUCCESS)
			{
				OnDeviceEvent(de);
			}
			break;
		}
		case MSG_NULL:
			break;
		default:
			std::cerr << "Error - Unknown DSM message " << msg << "." << std::endl;
			break;
		}
	}
}