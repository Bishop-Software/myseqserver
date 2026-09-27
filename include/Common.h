/*==============================================================================

	Copyright (C) 2006  All developers at http://sourceforge.net/projects/seq



	This program is free software; you can redistribute it and/or

	modify it under the terms of the GNU General Public License

	as published by the Free Software Foundation; either version 2

	of the License, or (at your option) any later version.



	This program is distributed in the hope that it will be useful,

	but WITHOUT ANY WARRANTY; without even the implied warranty of

	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the

	GNU General Public License for more details.



	You should have received a copy of the GNU General Public License

	along with this program; if not, write to the Free Software

	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

  ==============================================================================*/

#pragma once

#include <windows.h>

#include <string>

#include <iostream>

#include <strstream>

#include <vector>

#include <iomanip>

#include <sstream>

#include <algorithm>

#include <math.h>

#include <cstdint>

using namespace std;

using QWORD	 = uint64_t;
using PQWORD = uint64_t*;

#define EXCLEV_WARNING 1

#define EXCLEV_ERROR 2

// Default 64-bit eqgame.exe image/module base address. Offsets recorded in
// the .ini files are relative to this base; every offset read out of the
// target process gets rebased by subtracting this constant and adding the
// currently-attached process's actual base address (ASLR means the two
// rarely match). Kept as a single named constant so the ~20 call sites that
// do this arithmetic can't drift out of sync with each other.
static constexpr unsigned long long kEQImageBase = 0x140000000ULL;

class Exception : public string

{

	int level;

public:
	Exception(const int l, const string& s) :
		string(s),
		level(l)
	{
	}

	int getLevel() const
	{
		return level;
	}
};

// Minimal RAII owner for a Win32 HANDLE: closes it via CloseHandle when the
// guard goes out of scope, on every exit path, without needing a matching
// CloseHandle at each return statement. `invalidValue` lets callers specify
// the sentinel their API uses for "no handle" (NULL for most handles,
// INVALID_HANDLE_VALUE for CreateToolhelp32Snapshot/CreateFile) so valid()
// and the destructor never try to close the sentinel itself. Move-only,
// scope-bound guard -- not meant to be stored long-term; call release() to
// hand ownership to a caller/member that will manage its own lifetime.
class ScopedHandle
{
public:
	explicit ScopedHandle(HANDLE h = nullptr, HANDLE invalidValue = nullptr) :
		handle(h),
		invalid(invalidValue)
	{
	}

	~ScopedHandle()
	{
		close();
	}

	ScopedHandle(const ScopedHandle&)			 = delete;
	ScopedHandle& operator=(const ScopedHandle&) = delete;

	ScopedHandle(ScopedHandle&& other) noexcept :
		handle(other.handle),
		invalid(other.invalid)
	{
		other.handle = other.invalid;
	}

	ScopedHandle& operator=(ScopedHandle&& other) noexcept
	{
		if (this != &other)
		{
			close();
			handle		 = other.handle;
			invalid		 = other.invalid;
			other.handle = other.invalid;
		}
		return *this;
	}

	bool valid() const
	{
		return handle != invalid;
	}
	HANDLE get() const
	{
		return handle;
	}

	// Hands ownership to the caller; the destructor will no longer close it.
	HANDLE release()
	{
		HANDLE h = handle;
		handle	 = invalid;
		return h;
	}

private:
	void close()
	{
		if (valid())
			CloseHandle(handle);
	}

	HANDLE handle;
	HANDLE invalid;
};
