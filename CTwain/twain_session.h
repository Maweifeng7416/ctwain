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
	/// Contains event data after whatever data from the source has been transferred.
	/// </summary>
	struct TransferredDataEventArgs{
		/// <summary>
		/// Gets pointer to the complete data if the transfer was native.
		/// The data will be freed once the event handler ends
		/// so consumers must complete whatever processing before then.
		/// For image type this data is DIB (Windows) or TIFF (Linux).
		/// This pointer is already locked for the duration of this event.
		/// </summary>
		TW_HANDLE NativeData;

		/// <summary>
		/// Gets the final image information if applicable.
		/// </summary>
		/// <value>
		/// The final image information.
		/// </value>
		std::unique_ptr<TW_IMAGEINFO> ImageInfo;
	};

	/// <summary>
	/// The logical state of a TwainSession.
	/// </summary>
	enum class State{
		/// <summary>
		/// The starting state, corresponds to state 1.
		/// </summary>
		kDsmUnloaded = 1,
		/// <summary>
		/// The DSM library has been loaded, corresponds to state 2.
		/// </summary>
		kDsmLoaded = 2,
		/// <summary>
		/// The DSM has been opened, corresponds to state 3.
		/// </summary>
		kDsmOpened = 3,
		/// <summary>
		/// A data source has been opened, corresponds to state 4.
		/// </summary>
		kSourceOpened = 4,
		/// <summary>
		/// A data source has been enabled, corresponds to state 5.
		/// </summary>
		kSourceEnabled = 5,
		/// <summary>
		/// Data is ready for transfer from the source, corresponds to state 6.
		/// </summary>
		kTransferReady = 6,
		/// <summary>
		/// Data is being transferred, corresponds to state 7.
		/// </summary>
		kTransferring = 7
	};

	/// <summary>
	/// Basic class for interfacing with TWAIN. You should only have one of this per application process.
	/// </summary>
	class TwainSession
	{
	public:
		TwainSession(){}
		~TwainSession();
		// all disabled for now until everything is working
		TwainSession(const TwainSession&) = delete;            // Copy constructor
		TwainSession(TwainSession&&) = delete;                 // Move constructor
		TwainSession& operator=(const TwainSession&) = delete; // Copy assignment 
		TwainSession& operator=(TwainSession&&) = delete;      // move assignment 

		/// <summary>
		/// Gets the current logical state as defined by the TWAIN spec.
		/// </summary>
		/// <returns></returns>
		State state(){ return state_; }

		/// <summary>
		/// Initializes the data source manager. This must be the first method used
		/// before using other TWAIN functions. 
		/// </summary>
		bool Initialize();

		/// <summary>
		/// Forces the stepping down of an opened source when things gets out of control.
		/// Used when session state and source state become out of sync.
		/// </summary>
		void ForceStepDown(State state);


		/// <summary>
		/// Opens the data source manager. Calls to this must be followed by
		/// <see cref="CloseDsm" /> when done with a TWAIN session.
		/// </summary>
		/// <param name="window_handle">The window handle on Windows system to act as the parent.</param>
		TW_UINT16 OpenDsm(HWND window_handle = nullptr);

		/// <summary>
		/// Closes the data source manager.
		/// </summary>
		TW_UINT16 CloseDsm();

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
		State state_ = State::kDsmUnloaded;
		class MessageLoop* loop_ = nullptr;

		TW_USERINTERFACE ui_;
		TW_IDENTITY app_id_;
		TW_IDENTITY ds_id_;


		void DisableSource();
		void TryRegisterCallback();
		void HandleTransferReady();
		void TransferNative(bool image);
		void HandleDsmMessage(TW_UINT16);
	};
}

#endif //TWAIN_SESSION_H_