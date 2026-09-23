/*
 * Smart EQ Offset Finder - GPL Edition
 * Copyright 2007-2009, Carpathian <Carpathian01@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EQGAMESCANNER_H
#define EQGAMESCANNER_H

#include <string>
#include <vector>

#include <windows.h> // Provides "Windows Style" Type Definitions
#include "IniReader.h"
#include "NetworkServer.h"
#include "resource.h"

class EQGameScanner
{
public:
	EQGameScanner();
	~EQGameScanner();

public:
	bool executableExists() const;
	void setExe(const TCHAR* str);
	QWORD findEQPointerOffset(QWORD startAddress, std::size_t blockSize, const PBYTE byteMask, const PCHAR charMask);

	// Locates a RIP-relative reference (e.g. `lea reg, [rip+disp32]` /
	// `mov [rip+disp32], reg`) matching byteMask/charMask - the 4 bytes
	// marked 't' must be the instruction's disp32 field - and resolves it
	// to the absolute address it references (imageBase + instructionEndRVA
	// + disp32). This is the correct way to recover an absolute pointer
	// from x64 code, which references far data via RIP-relative
	// displacements rather than embedding the address as a literal, the
	// way 32-bit code did. Returns 0 if no match resolves into a mapped
	// section of the executable.
	QWORD findEQAbsolutePointer(QWORD startAddress, std::size_t blockSize, const PBYTE byteMask, const PCHAR charMask);

	bool ScanExecutable(HWND hDlg, IniReaderInterface* ir_intf, NetworkServerInterface* net_intf, bool write_out = false);
	void ScanSecondary(HWND hDlg, IniReaderInterface* ir_intf, NetworkServerInterface* net_intf);

private:
	bool compareData(PBYTE data, PBYTE byteMask, PCHAR charMask);

	struct PESection
	{
		QWORD virtualAddress;
		QWORD virtualSize;
		QWORD rawPointer;
		QWORD rawSize;
	};

	// Parses the DOS/NT/section headers of executablePath once, caching
	// the image base and section table needed to convert between file
	// offsets and RVAs. Returns false if the file isn't a valid PE32+
	// (x64) image.
	bool loadPEInfo();
	bool fileOffsetToRVA(QWORD fileOffset, QWORD& rva) const;
	bool rvaInMappedSection(QWORD rva) const;

private:
	std::string executablePath;

	bool peInfoLoaded{};
	QWORD imageBase{};
	std::vector<PESection> peSections;
};

#endif // EQGAMESCANNER_H
