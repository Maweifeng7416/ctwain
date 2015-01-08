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

#ifndef ENTRY_POINTS_H_
#define ENTRY_POINTS_H_


namespace ctwain{

	/// <summary>
	/// Contains all the function calls required to interop with TWAIN.
	/// This class should not be used by typical consumers.
	/// </summary>
	class EntryPoints
	{
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
		/// <param name="app_id">The pointer to application id.</param>
		static void UpdateMemoryEntry(pTW_IDENTITY app_id);

		/// <summary>
		/// Main DSM entry method.
		/// </summary>
		/// <param name="origin">The caller id.</param>
		/// <param name="destination">The destination id.</param>
		/// <param name="data_group">The DG_* value.</param>
		/// <param name="data_argument_type">The DAT_* value.</param>
		/// <param name="message">The MSG_* value.</param>
		/// <param name="data">The data pointer.</param>
		/// <returns></returns>
		static TW_UINT16 DSM_Entry(
			pTW_IDENTITY origin,
			pTW_IDENTITY destination,
			TW_UINT32    data_group,
			TW_UINT16    data_argument_type,
			TW_UINT16    message,
			TW_MEMREF    data);


		/// <summary>
		/// Function to allocate memory. Calls to this must be coupled with 
		/// <see cref="Free"/> later.
		/// </summary>
		/// <param name="size">The size in bytes.</param>
		/// <returns>Handle to the allocated memory.</returns>
		static TW_HANDLE Alloc(TW_UINT32 size);

		/// <summary>
		/// Function to free memory. 
		/// </summary>
		/// <param name="handle">The handle from <see cref="Allocate"/>.</param>
		static void Free(TW_HANDLE handle);

		/// <summary>
		/// Function to lock some memory. Calls to this must be coupled with 
		/// <see cref="Unlock"/> later.
		/// </summary>
		/// <param name="handle">The handle to allocated memory.</param>
		/// <returns>Handle to the lock.</returns>
		static TW_MEMREF Lock(TW_HANDLE handle);

		/// <summary>
		/// Function to unlock a previously locked memory region.
		/// </summary>
		/// <param name="handle">The handle from <see cref="Lock"/>.</param>
		static void Unlock(TW_HANDLE handle);

	private:
		static HMODULE dsm_module_;
		static DSMENTRYPROC dsm_entry_;
		static TW_ENTRYPOINT memory_entry_;
	};
}

#endif //ENTRY_POINTS_H_