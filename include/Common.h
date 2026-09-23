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

using QWORD = uint64_t;
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

	int getLevel() const { return level; }
};
