#pragma once

#include <memory>
#include <vector>
#include "twain2.3.h"


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

#ifndef _WINUSER_
struct tagMSG; // Forward or never
typedef tagMSG MSG;
#endif


/// <summary>
/// Basic class for interfacing with TWAIN. You should only have one of this per application process.
/// </summary>
[event_source(native)]
class TwainSession
{
private:
	class TwainSessionImpl* _pimpl;
public:
	TwainSession(); 
	TwainSession(const TwainSession&);            // Copy constructor
	TwainSession(TwainSession&&);                 // Move constructor
	TwainSession& operator=(const TwainSession&); // Copy assignment operator
	~TwainSession();

	virtual void FillAppId(TW_IDENTITY& appId);
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
	void OpenDsm(TW_MEMREF hWnd = nullptr);

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
	/// <param name="showUI">if set to <c>true</c> then show the driver UI.</param>
	TW_UINT16 EnableSource(bool modal, bool showUI);
	/// <summary>
	/// Shows the source's driver UI without transferring.
	/// </summary>
	/// <param name="modal">if set to <c>true</c> any driver UI will display as modal.</param>
	TW_UINT16 EnableSourceUIOnly(bool modal);

	/// <summary>
	/// Checks and handles the message if it's a TWAIN message
	/// from inside a Windows message loop.
	/// </summary>
	bool IsTwainMessage(const MSG& message);

	/// <summary>
	/// Raw dsm entry call using current source.
	/// </summary>
	TW_UINT16 DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);

	/// <summary>
	/// Occurs when the source has generated an event.
	/// </summary>
	__event void DeviceEvent(const TW_DEVICEEVENT& deviceEvent);
	/// <summary>
	/// Occurs when a data transfer is ready.
	/// </summary>
	__event void TransferReady(TransferReadyEventArgs& readyEvent);
};

