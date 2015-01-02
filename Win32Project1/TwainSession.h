#pragma once
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
	pTW_IMAGEINFO PendingImageInfo;

	/// <summary>
	/// Gets the audio information for the current transfer if applicable.
	/// </summary>
	pTW_AUDIOINFO AudioInfo;
};

/// <summary>
/// Basic class for interfacing with TWAIN. You should only have one of this per application process.
/// </summary>
[event_source(native)]
class TwainSession
{
private:
	HMODULE _dsmModule = nullptr;
	TW_IDENTITY _appId;
	TW_IDENTITY _srcId;
	DSMENTRYPROC _dsmEntry = nullptr;
	HWND _handle = nullptr;
	TW_USERINTERFACE _ui;
	int _state = 1;
	void FillAppId();
	void DisableSource();
	void ForceStepDown(int state);
	void DoTransfer();
public:
	TwainSession();
	~TwainSession();

	/// <summary>
	/// Gets the current state number as defined by the TWAIN spec.
	/// </summary>
	/// <returns></returns>
	int GetState(){ return _state; }

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

	void OpenDsm(HWND handle);
	void CloseDsm();

	TW_STATUS GetDsmStatus();
	TW_STATUS GetSourceStatus();

	TW_IDENTITY ShowSourceSelector();
	TW_UINT16 OpenSource(TW_IDENTITY& srcId);
	TW_UINT16 CloseSource();
	TW_UINT16 EnableSource(bool modal, bool showUI);
	TW_UINT16 EnableSourceUIOnly(bool modal);

	bool IsTwainMessage(const MSG* message);
	TW_UINT16 DsmEntry(TW_UINT32 DG, TW_UINT16 DAT, TW_UINT16 MSG, TW_MEMREF pData);

	__event void DeviceEvent(const TW_DEVICEEVENT& deviceEvent);
	__event void TransferReady(TransferReadyEventArgs& e);
};

