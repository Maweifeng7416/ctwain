#pragma once
#include "twain2.3.h"
#include <memory>
#include <vector>


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
[event_source(native)]
class TwainSession
{
private:
	HMODULE _dsmModule = nullptr;
	DSMENTRYPROC _dsmEntry = nullptr;
	HWND _handle = nullptr;
	int _state = 1;

	TW_IDENTITY _appId;
	TW_IDENTITY _srcId;
	TW_USERINTERFACE _ui;

	void FillAppId();
	void DisableSource();
	void DoTransfer();
	TW_UINT16 TW_CALLINGSTYLE DsmCallback(pTW_IDENTITY, pTW_IDENTITY, TW_UINT32, TW_UINT16, TW_UINT16, TW_MEMREF);
	void HandleDsmMessage(TW_UINT16);
public:
	TwainSession();
	~TwainSession();

	/// <summary>
	/// Forces the stepping down of an opened source when things gets out of control.
	/// Used when session state and source state become out of sync.
	/// </summary>
	void ForceStepDown(int state);

	/// <summary>
	/// Gets the current state number as defined by the TWAIN spec.
	/// </summary>
	/// <returns></returns>
	int GetState(){ return _state; }

	/// <summary>
	/// Quick flag to check if the DSM dll has been loaded.
	/// </summary>
	bool IsDsmInitialized(){ return _state > 1; }

	/// <summary>
	/// Quick flag to check if the DSM has been opened.
	/// </summary>
	bool IsDsmOpen(){ return _state > 2; }

	/// <summary>
	/// Quick flag to check if a source has been opened.
	/// </summary>
	bool IsSourceOpen(){ return _state > 3; }

	/// <summary>
	/// Quick flag to check if a source has been enabled.
	/// </summary>
	bool IsSourceEnabled(){ return _state > 4; }

	/// <summary>
	/// Quick flag to check if a source is in the transferring state.
	/// </summary>
	bool IsTransferring(){ return _state > 5; }

	/// <summary>
	/// Initializes the data source manager. This must be the first method used
	/// before using other TWAIN functions. 
	/// </summary>
	bool Initialize();

	/// <summary>
	/// Opens the data source manager. Calls to this must be followed by
	/// <see cref="CloseDsm" /> when done with a TWAIN session.
	/// </summary>
	void OpenDsm(HWND handle);

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
	/// Checks and handles the message if it's a TWAIN message.
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

