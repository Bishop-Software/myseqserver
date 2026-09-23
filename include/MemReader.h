/*==============================================================================

	Copyright (C) 2006-2013  All developers at http://sourceforge.net/projects/seq

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

#include "Common.h"

#include "IniReader.h"

#include <tlhelp32.h>

// The interface classes can be extended, but never changed! They force backwards compatibility.

class MemReaderInterface

{

public:
	virtual bool isValid() = 0;

	virtual bool openFirstProcess(const string& filename, bool debug = false) = 0;

	virtual bool openNextProcess(const string& filename, bool debug = false) = 0;

	virtual QWORD extractPointer(QWORD offset) = 0;

	virtual QWORD extractRAWPointer(QWORD offset) = 0;

	virtual string extractString(QWORD offset) = 0;

	virtual string extractString2(QWORD offset) = 0;

	virtual bool extractToBuffer(QWORD offset, char* buffer, UINT size) = 0;

	virtual DWORD getCurrentPID() = 0;

	virtual QWORD getCurrentBaseAddress() = 0;

	virtual HANDLE getCurrentHandle() = 0;

	virtual float extractFloat(QWORD offset) = 0;

	virtual BYTE extractBYTE(QWORD offset) = 0;

	virtual UINT extractUINT(QWORD offset) = 0;
};

class MemReader : public MemReaderInterface

{

string originalFilename;

	// HANDLE 	currentEQProcessHandle;

	// DWORD	currentEQProcessID;

	// DWORD	currentEQProcessBaseAddress;

	UINT readCount;

	bool openProcess(string filename, bool first, bool debug);

protected:
	HANDLE currentEQProcessHandle;

	DWORD currentEQProcessID;

	QWORD currentEQProcessBaseAddress;

public:
	MemReader();

	~MemReader();

	bool isValid() override;

	void enableDebugPrivileges();

	bool openFirstProcess(const string& filename, bool debug = false) override;

	bool openNextProcess(const string& filename, bool debug = false) override;

	void closeProcess();

	bool validateProcess(bool forceCheck);

	QWORD extractPointer(QWORD offset) override;

	QWORD extractRAWPointer(QWORD offset) override;

	string extractString(QWORD offset) override;

	string extractString2(QWORD offset) override;

	bool extractToBuffer(QWORD offset, char* buffer, UINT size) override;

	DWORD getCurrentPID() override;

	QWORD getCurrentBaseAddress() override;

	HANDLE getCurrentHandle() override;

	float extractFloat(QWORD offset) override;

	BYTE extractBYTE(QWORD offset) override;

	UINT extractUINT(QWORD offset) override;

	bool AdjustPrivileges();

	QWORD GetModuleBaseAddress(DWORD iProcId, TCHAR* DLLName);
};
