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
#ifndef TWAIN_SESSION_H_
#define TWAIN_SESSION_H_


#include <memory>
#include <vector>
#include "twain2.3.h"

namespace ctwain{

	/// <summary>
	/// Indicates the mode to enable the TWAIN source with.
	/// </summary>
	enum class EnableSourceMode{
		/// <summary>
		/// Show driver UI during data transfer.
		/// </summary>
		kShowUI,
		/// <summary>
		/// Show driver UI without transferring data.
		/// </summary>
		kShowUIOnly,
		/// <summary>
		/// Hide driver UI during data transfer.
		/// </summary>
		kHideUI
	};

	/// <summary>
	/// Contains event data when a data transfer is ready to be processed.
	/// </summary>
	struct TransferReadyEventArgs
	{
		/// <summary>
		/// Gets or sets a value indicating whether the current transfer should be canceled
		/// and continue next transfer if there are more data.
		/// </summary>
		bool CancelCurrent;

		/// <summary>
		/// Gets or sets a value indicating whether all transfers should be canceled.
		/// </summary>
		bool CancelAll;

		/// <summary>
		/// Gets a value indicating whether current transfer signifies an end of job in TWAIN world.
		/// </summary>
		bool EndOfJob;

		/// <summary>
		/// Gets the known pending transfer count. This may not be appilicable 
		/// for certain scanning modes.
		/// </summary>
		int PendingTransferCount;

		/// <summary>
		/// Gets the tentative image information for the current transfer if applicable.
		/// This may differ from the final image depending on the transfer mode used (mostly when doing mem xfer).
		/// </summary>
		std::unique_ptr<TW_IMAGEINFO> PendingImageInfo;

		/// <summary>
		/// Gets the audio information for the current transfer if applicable.
		/// </summary>
		std::unique_ptr<TW_AUDIOINFO> AudioInfo;
	};

	/// <summary>
	/// Contains event data on the current data transfer.
	/// </summary>
	struct TransferredDataEventArgs{		
		/// <summary>
		/// The data from native transfer.
		/// </summary>
		TW_HANDLE NativeData;
	};

	/// <summary>
	/// Basic class for interfacing with TWAIN. You should only have one of this per application process.
	/// </summary>
	class TwainSession
	{
	public:
		TwainSession() :state_{ 1 }, loop_{ nullptr }{}
		~TwainSession();
		// all disabled for now until everything is working
		TwainSession(const TwainSession&)=delete;            // Copy constructor
		TwainSession(TwainSession&&)=delete;                 // Move constructor
		TwainSession& operator=(const TwainSession&)=delete; // Copy assignment 
		TwainSession& operator=(TwainSession&&)=delete;      // move assignment 

		/// <summary>
		/// Gets the current state number as defined by the TWAIN spec.
		/// </summary>
		/// <returns></returns>
		int state(){ return state_; }

		/// <summary>
		/// Quick flag to check if the DSM dll has been loaded.
		/// </summary>
		bool IsDsmInitialized(){ return state_ > 1; }

		/// <summary>
		/// Quick flag to check if the DSM has been opened.
		/// </summary>
		bool IsDsmOpen(){ return state_ > 2; }

		/// <summary>
		/// Quick flag to check if a source has been opened.
		/// </summary>
		bool IsSourceOpen(){ return state_ > 3; }

		/// <summary>
		/// Quick flag to check if a source has been enabled.
		/// </summary>
		bool IsSourceEnabled(){ return state_ > 4; }

		/// <summary>
		/// Quick flag to check if a source is in the transferring state.
		/// </summary>
		bool IsTransferring(){ return state_ > 5; }

		/// <summary>
		/// Initializes the data source manager. This must be the first method used
		/// before using other TWAIN functions. 
		/// </summary>
		bool Initialize();

		/// <summary>
		/// Forces the stepping down of an opened source when things gets out of control.
		/// Used when session state and source state become out of sync.
		/// </summary>
		void ForceStepDown(int state);


		/// <summary>
		/// Opens the data source manager. Calls to this must be followed by
		/// <see cref="CloseDsm" /> when done with a TWAIN session.
		/// </summary>
		/// <param name="window_handle">The window handle on Windows system to act as the parent.</param>
		void OpenDsm(HWND window_handle = nullptr);

		/// <summary>
		/// Closes the data source manager.
		/// </summary>
		void CloseDsm();

		/// <summary>
		/// Gets the manager status. Only call this at state 2 or higher.
		/// </summary>
		TW_STATUS GetDsmStatus();
		/// <summary>
		/// Gets the source status. Only call this at state 4 or higher.
		/// </summary>
		TW_STATUS GetSourceStatus();

		/// <summary>
		/// Try to show the built-in source selector dialog and return the selected source.
		/// </summary>
		TW_IDENTITY ShowSourceSelector();
		/// <summary>
		/// Gets the default source for this application.
		/// </summary>
		TW_IDENTITY GetDefaultSource();
		/// <summary>
		/// Gets list of sources available in the system.
		/// Only call this at state 2 or higher.
		/// </summary>
		std::vector<TW_IDENTITY> GetSources();

		/// <summary>
		/// Opens the source for capability negotiation.
		/// <param name="source">The source id to open.</param>
		/// </summary>
		TW_UINT16 OpenSource(TW_IDENTITY& source);
		/// <summary>
		/// Closes the source.
		/// </summary>
		TW_UINT16 CloseSource();
		/// <summary>
		/// Enables the source to start transferring.
		/// </summary>
		/// <param name="modal">if set to <c>true</c> any driver UI will display as modal.</param>
		/// <param name="mode">indicate the enable mode.</param>
		TW_UINT16 EnableSource(EnableSourceMode mode, bool modal);


		/// <summary>
		/// Checks and handles the message if it's a TWAIN message
		/// from inside a Windows message loop.
		/// </summary>
		/// <param name="message">The message from Windows message loop.</param>
		bool IsTwainMessage(const MSG& message);

		/// <summary>
		/// Raw dsm entry call using current application id.
		/// </summary>
		/// <param name="includeSource">Whether to pass the current source id.</param>
		/// <param name="data_group">The DG_* value.</param>
		/// <param name="data_argument_type">The DAT_* value.</param>
		/// <param name="message">The MSG_* value.</param>
		/// <param name="data">The data pointer.</param>
		/// <returns></returns>
		TW_UINT16 DsmEntry(
			bool includeSource,
			TW_UINT32    data_group,
			TW_UINT16    data_argument_type,
			TW_UINT16    message,
			TW_MEMREF    data);



		/// <summary>
		/// Called when the application id needs to be populated.
		/// </summary>
		/// <param name="appId">The application id to be used during TWAIN calls.</param>
		virtual void OnFillAppId(TW_IDENTITY& appId);

		/// <summary>
		/// Called when the source has generated an event.
		/// </summary>
		/// <param name="deviceEvent">The TWAIN device event object.</param>
		virtual void OnDeviceEvent(const TW_DEVICEEVENT& deviceEvent){}

		/// <summary>
		/// Called when a data transfer is ready.
		/// </summary>
		/// <param name="readyEvent">The event object for controlling the upcoming transfer.</param>
		virtual void OnTransferReady(TransferReadyEventArgs& readyEvent){}

		/// <summary>
		/// Called when data has been transferred from the source.
		/// </summary>
		/// <param name="transferEvent">The transfer event.</param>
		virtual void OnTransferredData(const TransferredDataEventArgs& transferEvent){}

		/// <summary>
		/// Called when the source has been disabled.
		/// </summary>
		virtual void OnSourceDisabled(){}

	private:
		int state_;
		class MessageLoop* loop_; // test

		TW_USERINTERFACE ui_;
		TW_IDENTITY app_id_;
		TW_IDENTITY ds_id_;


		void DisableSource();
		void TryRegisterCallback();
		void DoTransfer();
		void HandleDsmMessage(TW_UINT16);
	};
}

#endif //TWAIN_SESSION_H_