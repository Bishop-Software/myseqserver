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

class IniReaderInterface

{

public:
	virtual void openFile(const string& filename) = 0;

	virtual void openConfigFile(const string& filename) = 0;

	virtual string readStringEntry(const string& section, const string& entry, bool config = false) = 0;

	virtual QWORD readIntegerEntry(const string& section, const string& entry, bool config = false) = 0;

	virtual bool writeStringEntry(const string& section, const string& entry, const string& value, bool config = false) = 0;

	virtual string readEscapeStrings(const string& section, const string& entry) = 0;
};

class IniReader : public IniReaderInterface

{
public:
	IniReader();

	~IniReader();

private:
	string filename;

	string configfilename;

	_TCHAR buffer[255]{};

	bool StartMinimized;

public:
	void openFile(const string& filename) override;

	void openConfigFile(const string& filename) override;

	string readStringEntry(const string& section, const string& entry, bool config = false) override;

	string readEscapeStrings(const string& section, const string& entry) override;

	QWORD readIntegerEntry(const string& section, const string& entry, bool config = false) override;

	bool writeStringEntry(const string& section, const string& entry, const string& value, bool config = false) override;

	string GetPatchDate();

	string patchDate;

	bool GetStartMinimized()
	{
		return StartMinimized;
	}

	void ToggleStartMinimized();

private:
	void SetStartMinimized(bool value)
	{
		StartMinimized = value;
	}
};
