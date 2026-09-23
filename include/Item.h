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

struct itemBuffer_t

{

	DWORD id{};

	DWORD dropid{};

	float x{};

	float y{};

	float z{};

	string name;

	UINT flags{};
};

class Item

{

public:
	enum offset_types
	{
		OT_prev,
		OT_next,
		OT_id,
		OT_dropid,
		OT_x,
		OT_y,
		OT_z,
		OT_name,
		OT_max
	};

	UINT offsets[OT_max]{};

	UINT largestOffset;

	string offsetNames[OT_max];

	char* rawBuffer{};

	itemBuffer_t tempItemBuffer;

	vector<itemBuffer_t> itemList;

private:
	string extractRawString(const offset_types ot) const
	{
		const UINT start = offsets[ot];

		const UINT maxLen = (start < largestOffset) ? (largestOffset - start) : 0;

		const char* p = &rawBuffer[start];

		UINT len = 0;

		while (len < maxLen && p[len] != '\0')
			len++;

		return string(p, len);
	}

	float extractRawFloat(const offset_types ot) const
	{
		return *reinterpret_cast<float*>(&rawBuffer[offsets[ot]]);
	}

	DWORD extractRawDWord(const offset_types ot) const
	{
		return *reinterpret_cast<DWORD*>(&rawBuffer[offsets[ot]]);
	}

	QWORD extractRawQWord(offset_types ot) const
	{
		return *reinterpret_cast<QWORD*>(&rawBuffer[offsets[ot]]);
	}

	BYTE extractRawByte(const offset_types ot) const
	{
		return *reinterpret_cast<BYTE*>(&rawBuffer[offsets[ot]]);
	}

public:
	Item();
	~Item();

	Item(const Item&) = delete;
	Item& operator=(const Item&) = delete;

	void init(IniReaderInterface* ir_intf);

	void setOffset(offset_types ot, int value, string name);

	void packItemBuffer(UINT flags);

	/* when you are done filling out a NetBuffer, push it for shipping across the network */

	void pushNetBuffer()
	{
		itemList.push_back(tempItemBuffer);
	}

	UINT getNetBufferSize() const
	{
		return static_cast<UINT>(itemList.size());
	}

	itemBuffer_t* getNetBufferStart()
	{
		return &itemList.front();
	}

	/* when you are done shipping all the data across the network, reset/clear the NetBuffers */

	void clearNetBuffer()
	{
		itemList.clear();
	}

	QWORD extractNextPointer() const
	{
		return extractRawQWord(OT_next);
	}

	QWORD extractPrevPointer() const
	{
		return extractRawQWord(OT_prev);
	}
};
