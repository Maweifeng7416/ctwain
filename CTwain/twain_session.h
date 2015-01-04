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
#include "twain_interop.h"

namespace ctwain{

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
		std::unique_ptr<pTW_AUDIOINFO> AudioInfo;
	};



	/// <summary>
	/// Basic class for interfacing with TWAIN. You should only have one of this per application process.
	/// </summary>
	//[event_source(native)]
	class TwainSession
	{
	public:
		TwainSession();
		TwainSession(const TwainSession&);            // Copy constructor
		TwainSession(TwainSession&&);                 // Move constructor
		TwainSession& operator=(const TwainSession&); // Copy assignment operator
		~TwainSession();

		virtual void FillAppId(TW_IDENTITY& app_id);

		/// <summary>
		/// Gets the current state number as defined by the TWAIN spec.
		/// </summary>
		/// <returns></returns>
		int GetState();

		/// <summary>
		/// Quick flag to check if the DSM dll has been loaded.
		/// </summary>
		bool IsDsmInitialized();

		/// <summary>
		/// Quick flag to check if the DSM has been opened.
		/// </summary>
		bool IsDsmOpen();

		/// <summary>
		/// Quick flag to check if a source has been opened.
		/// </summary>
		bool IsSourceOpen();

		/// <summary>
		/// Quick flag to check if a source has been enabled.
		/// </summary>
		bool IsSourceEnabled();

		/// <summary>
		/// Quick flag to check if a source is in the transferring state.
		/// </summary>
		bool IsTransferring();

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
		void OpenDsm(TW_MEMREF window_handle = nullptr);

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
		std::unique_ptr<TW_IDENTITY> ShowSourceSelector();
		/// <summary>
		/// Gets the default source for this application.
		/// </summary>
		std::unique_ptr<TW_IDENTITY> GetDefaultSource();
		/// <summary>
		/// Gets list of sources available in the system.
		/// Only call this at state 2 or higher.
		/// </summary>
		std::vector<std::unique_ptr<TW_IDENTITY>> GetSources();

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
		/// <param name="show_ui">if set to <c>true</c> then show the driver UI.</param>
		TW_UINT16 EnableSource(bool modal, bool show_ui);
		/// <summary>
		/// Shows the source's driver UI without transferring.
		/// </summary>
		/// <param name="modal">if set to <c>true</c> any driver UI will display as modal.</param>
		TW_UINT16 EnableSourceUIOnly(bool modal);

		/// <summary>
		/// Checks and handles the message if it's a TWAIN message
		/// from inside a Windows message loop.
		/// </summary>
		/// <param name="message">The message from Windows message loop.</param>
		bool IsTwainMessage(const MSG& message);

		/// <summary>
		/// Raw dsm entry call using current source.
		/// </summary>
		/// <param name="data_group">The DG_* value.</param>
		/// <param name="data_argument_type">The DAT_* value.</param>
		/// <param name="message">The MSG_* value.</param>
		/// <param name="data">The data pointer.</param>
		/// <returns></returns>
		TW_UINT16 DsmEntry(
			TW_UINT32    data_group,
			TW_UINT16    data_argument_type,
			TW_UINT16    message,
			TW_MEMREF    data);

		/*
			/// <summary>
			/// Occurs when the source has generated an event.
			/// </summary>
			__event void DeviceEvent(const TW_DEVICEEVENT& deviceEvent);
			/// <summary>
			/// Occurs when a data transfer is ready.
			/// </summary>
			__event void TransferReady(TransferReadyEventArgs& readyEvent);*/

	private:
		class TwainSessionImpl* pimpl_;
	};
}

#endif //TWAIN_SESSION_H_