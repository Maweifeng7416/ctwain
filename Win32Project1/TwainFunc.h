#pragma once

#include "TwainInterop.h"

/// <summary>
/// Contains all the TWAIN-related function calls.
/// </summary>
class TwainFunc
{
private:
	static HMODULE _dsmModule;
	static DSMENTRYPROC _dsmEntry;
	static TW_ENTRYPOINT _memEntry;
public:	
	/// <summary>
	/// Loads the DSM library.
	/// </summary>
	/// <returns></returns>
	static bool InitializeDSM();
	/// <summary>
	/// Unloads the DSM library.
	/// </summary>
	static void UninitializeDSM();

	/// <summary>
	/// Updates the memory management function entry with current app id.
	/// </summary>
	/// <param name="entryPoint">The entry point.</param>
	static void UpdateMemoryEntry(pTW_IDENTITY appId);

	/// <summary>
	/// Main DSM entry method.
	/// </summary>
	/// <param name="pOrigin">The origin.</param>
	/// <param name="pDest">The dest.</param>
	/// <param name="DG">The DG value.</param>
	/// <param name="DAT">The DAT value.</param>
	/// <param name="MSG">The MSG value.</param>
	/// <param name="pData">The data.</param>
	/// <returns></returns>
	static TW_UINT16 DSM_Entry(
		pTW_IDENTITY pOrigin,
		pTW_IDENTITY pDest,
		TW_UINT32    DG,
		TW_UINT16    DAT,
		TW_UINT16    MSG,
		TW_MEMREF    pData);
	

	/// <summary>
	/// Function to allocate memory. Calls to this must be coupled with <see cref="Free"/> later.
	/// </summary>
	/// <param name="size">The size in bytes.</param>
	/// <returns>Handle to the allocated memory.</returns>
	TW_HANDLE Alloc(TW_UINT32 size);

	/// <summary>
	/// Function to free memory. 
	/// </summary>
	/// <param name="handle">The handle from <see cref="Allocate"/>.</param>
	void Free(TW_HANDLE handle);

	/// <summary>
	/// Function to lock some memory. Calls to this must be coupled with <see cref="Unlock"/> later.
	/// </summary>
	/// <param name="handle">The handle to allocated memory.</param>
	/// <returns>Handle to the lock.</returns>
	TW_MEMREF Lock(TW_HANDLE handle);

	/// <summary>
	/// Function to unlock a previously locked memory region.
	/// </summary>
	/// <param name="handle">The handle from <see cref="Lock"/>.</param>
	void Unlock(TW_HANDLE handle);
};

