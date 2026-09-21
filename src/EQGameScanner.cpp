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
#include "stdafx.h"
#include <fstream>
#include "EQGameScanner.h"
#include "minwindef.h"

typedef uint64_t* PQWORD;

/*
 * Offset Value Storage
 *
 * Holds the resolved CharInfo pointer for the duration of a ScanSecondary
 * pass, used as a confidence signal only (see ScanSecondary) - the
 * SpawnInfo* patterns themselves are already fully concrete byte
 * signatures and don't need it as an input.
 */
namespace EQPrimaryOffsets
{
QWORD CharInfo = 0x0;
}; // namespace EQPrimaryOffsets

namespace
{
// The six [Memory Offsets] entries: absolute pointers resolved via
// findEQAbsolutePointer (RIP-relative disp32 resolution) and compared
// against/written back to their matching NetworkServer offset slot.
struct PrimaryOffsetEntry
{
	const char* iniSection;
	NetworkServer::offset_types offsetType;
};

const PrimaryOffsetEntry kPrimaryOffsets[] = {
	{"ZoneAddr", NetworkServer::OT_zonename},
	{"SpawnHeaderAddr", NetworkServer::OT_spawnlist},
	{"CharInfo", NetworkServer::OT_self},
	{"ItemsAddr", NetworkServer::OT_ground},
	{"TargetAddr", NetworkServer::OT_target},
	{"WorldAddr", NetworkServer::OT_world},
};

// The SpawnInfo struct-member offsets: plain immediate displacements
// resolved via findEQPointerOffset, reported but not written back to the
// ini automatically (ScanSecondary has always been a read-only report).
struct SpawnInfoOffsetEntry
{
	const char* iniSection;
	const char* displayName;
};

const SpawnInfoOffsetEntry kSpawnInfoOffsets[] = {
	{"SpawnInfoNextOffset", "NextOffset"},
	{"SpawnInfoPrevOffset", "PrevOffset"},
	{"SpawnInfoLastnameOffset", "LastnameOffset"},
	{"SpawnInfoXOffset", "XOffset"},
	{"SpawnInfoYOffset", "YOffset"},
	{"SpawnInfoZOffset", "ZOffset"},
	{"SpawnInfoSpeedOffset", "SpeedOffset"},
	{"SpawnInfoHeadingOffset", "HeadingOffset"},
	{"SpawnInfoNameOffset", "NameOffset"},
	{"SpawnInfoTypeOffset", "TypeOffset"},
	{"SpawnInfoSpawnIDOffset", "SpawnIDOffset"},
	{"SpawnInfoOwnerIDOffset", "OwnerIDOffset"},
	{"SpawnInfoHideOffset", "HideOffset"},
	{"SpawnInfoLevelOffset", "LevelOffset"},
	{"SpawnInfoRaceOffset", "RaceOffset"},
	{"SpawnInfoClassOffset", "ClassOffset"},
	{"SpawnInfoPrimaryOffset", "PrimaryOffset"},
	{"SpawnInfoOffhandOffset", "OffhandOffset"},
};
} // namespace

EQGameScanner::EQGameScanner(void)
{
}

EQGameScanner::~EQGameScanner(void)
{
}
void EQGameScanner::setExe(TCHAR* str)
{
	executablePath = str;
}
bool EQGameScanner::executableExists() const
{
	std::ifstream file(executablePath.c_str(), std::ios::in);

	if (file)
	{
		file.close();
		return true;
	}

	return false;
}

QWORD EQGameScanner::findEQPointerOffset(QWORD startAddress, std::size_t blockSize, const PBYTE byteMask, const PCHAR charMask)
{
	std::ifstream file(executablePath.c_str(), std::ios::in | std::ios::binary);

	// If the file can't be opened, return NULL for pointer offset.
	if (!file)
		return NULL;

	int typelen = 0;

	typelen = (int)std::string(charMask).find_last_of("t") - (int)std::string(charMask).find_first_of("t") + 1;

	if (typelen < 1)
		typelen = 4;

	// Setup our temporary storage variables
	PBYTE buffer	= new BYTE[blockSize];
	QWORD matchAddr = NULL;

	// I like clean memory.
	memset(buffer, 0, blockSize);

	// Move get pointer to the start of the block we want to search
	// Then attempt to read blockSize to the buffer
	file.seekg(startAddress, std::ios::beg);
	file.read((char*)buffer, blockSize);

	// Search for a position that fits our masks in memory.
	// Thanks to dom1n1k for the piece of code this is based off of.
	for (QWORD i = 0; i < blockSize; ++i)
	{
		if (compareData(buffer + i, byteMask, charMask))
		{
			QWORD checkRet;
			matchAddr = i;
			if (typelen == 1)
			{
				BYTE chechbyteRet = *reinterpret_cast<PBYTE>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
				checkRet		  = (QWORD)chechbyteRet;
			}
			else if (typelen == 2)
			{
				WORD checkwordRet = *reinterpret_cast<PWORD>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
				checkRet		  = (QWORD)checkwordRet;
			}
			else if (typelen >= 8)
			{
				checkRet = *reinterpret_cast<PQWORD>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
			}
			else
			{
				checkRet = *reinterpret_cast<PDWORD>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
			}
			// DWORD checkRet = *reinterpret_cast<PDWORD>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
			if (checkRet < 536870912)
				break;
			else
				matchAddr = NULL;
		}
	}

	// If we didn't find a match, return NULL
	if (matchAddr == NULL)
		return NULL;

	QWORD nRet;

	// Find where our target address we're searching for is stored, and return its value.
	if (typelen == 1)
	{
		BYTE cRet = *reinterpret_cast<PBYTE>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
		nRet	  = (QWORD)cRet;
	}
	else if (typelen == 2)
	{
		WORD wRet = *reinterpret_cast<PWORD>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
		nRet	  = (QWORD)wRet;
	}
	else if (typelen >= 8)
	{
		nRet = *reinterpret_cast<PQWORD>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
	}
	else
	{
		nRet = *reinterpret_cast<PDWORD>(buffer + matchAddr + std::string(charMask).find_first_of("t"));
	}
	delete[] buffer;

	return nRet;
}

bool EQGameScanner::loadPEInfo()
{
	if (peInfoLoaded)
		return true;

	std::ifstream file(executablePath.c_str(), std::ios::in | std::ios::binary);
	if (!file)
		return false;

	BYTE dosHeader[64];
	file.read((char*)dosHeader, sizeof(dosHeader));
	if (file.gcount() != sizeof(dosHeader) || dosHeader[0] != 'M' || dosHeader[1] != 'Z')
		return false;

	DWORD e_lfanew = *reinterpret_cast<DWORD*>(dosHeader + 0x3C);

	file.seekg(e_lfanew, std::ios::beg);
	BYTE peSig[4];
	file.read((char*)peSig, sizeof(peSig));
	if (peSig[0] != 'P' || peSig[1] != 'E' || peSig[2] != 0 || peSig[3] != 0)
		return false;

	BYTE coffHeader[20];
	file.read((char*)coffHeader, sizeof(coffHeader));
	WORD numSections			  = *reinterpret_cast<WORD*>(coffHeader + 2);
	WORD sizeOfOptionalHeader	  = *reinterpret_cast<WORD*>(coffHeader + 16);
	std::streamoff optHeaderStart = file.tellg();

	WORD magic;
	file.read((char*)&magic, sizeof(magic));
	if (magic != 0x20B) // PE32+ (x64) only - this repo no longer supports 32-bit clients
		return false;

	// ImageBase sits at offset 24 within the PE32+ optional header.
	file.seekg(optHeaderStart + 24, std::ios::beg);
	QWORD base = 0;
	file.read((char*)&base, sizeof(base));
	imageBase = base;

	file.seekg(optHeaderStart + sizeOfOptionalHeader, std::ios::beg);

	peSections.clear();
	for (WORD i = 0; i < numSections; i++)
	{
		BYTE sectionHeader[40];
		file.read((char*)sectionHeader, sizeof(sectionHeader));

		PESection sec;
		sec.virtualSize	= *reinterpret_cast<DWORD*>(sectionHeader + 8);
		sec.virtualAddress = *reinterpret_cast<DWORD*>(sectionHeader + 12);
		sec.rawSize			= *reinterpret_cast<DWORD*>(sectionHeader + 16);
		sec.rawPointer		= *reinterpret_cast<DWORD*>(sectionHeader + 20);
		peSections.push_back(sec);
	}

	peInfoLoaded = true;
	return true;
}

bool EQGameScanner::fileOffsetToRVA(QWORD fileOffset, QWORD& rva) const
{
	for (const auto& sec : peSections)
	{
		if (fileOffset >= sec.rawPointer && fileOffset < sec.rawPointer + sec.rawSize)
		{
			rva = sec.virtualAddress + (fileOffset - sec.rawPointer);
			return true;
		}
	}
	return false;
}

bool EQGameScanner::rvaInMappedSection(QWORD rva) const
{
	for (const auto& sec : peSections)
	{
		if (rva >= sec.virtualAddress && rva < sec.virtualAddress + sec.virtualSize)
			return true;
	}
	return false;
}

QWORD EQGameScanner::findEQAbsolutePointer(QWORD startAddress, std::size_t blockSize, const PBYTE byteMask, const PCHAR charMask)
{
	if (!loadPEInfo())
		return 0;

	std::string mask(charMask);
	size_t tPos = mask.find_first_of("t");
	size_t tLen = mask.find_last_of("t") - tPos + 1;
	if (tPos == std::string::npos || tLen != 4)
		return 0; // this resolver only understands a 4-byte disp32 capture

	std::ifstream file(executablePath.c_str(), std::ios::in | std::ios::binary);
	if (!file)
		return 0;

	PBYTE buffer = new BYTE[blockSize];
	memset(buffer, 0, blockSize);

	file.seekg(startAddress, std::ios::beg);
	file.read((char*)buffer, blockSize);

	QWORD result = 0;

	for (QWORD i = 0; i + mask.size() <= blockSize; i++)
	{
		if (!compareData(buffer + i, byteMask, charMask))
			continue;

		INT32 disp = *reinterpret_cast<INT32*>(buffer + i + tPos);

		QWORD rvaAfterField;
		if (!fileOffsetToRVA(startAddress + i + tPos + 4, rvaAfterField))
			continue;

		QWORD candidate	= imageBase + rvaAfterField + disp;
		QWORD candidateRva = candidate - imageBase;

		// A genuine RIP-relative reference must resolve into a mapped
		// section; this also weeds out incidental byte-pattern matches.
		if (!rvaInMappedSection(candidateRva))
			continue;

		result = candidate;
		break;
	}

	delete[] buffer;
	return result;
}

// Thanks to dom1n1k for the piece of code this is based off of.
bool EQGameScanner::compareData(PBYTE data, PBYTE byteMask, PCHAR charMask)
{
	for (; *charMask; ++charMask, ++data, ++byteMask)
	{
		if ((*charMask == 'x' || *charMask == 'o') && *data != *byteMask)
			return false;
	}
	return (*charMask) == NULL;
}

bool EQGameScanner::ScanExecutable(HWND hDlg, IniReaderInterface* ir_intf, NetworkServerInterface* net_intf, bool write_out)
{
	if (!executableExists())
	{
		SetDlgItemText(hDlg, IDC_EDIT2, "Error: Could not locate the specified executable file.");
		return false;
	}
	bool reload = false;

	std::ostringstream outputStream;

	WIN32_FILE_ATTRIBUTE_DATA FileData = {0};
	if (GetFileAttributesEx(executablePath.c_str(), GetFileExInfoStandard, &FileData))
	{
		TCHAR szFileDate[255];
		FILETIME ftLastMod = FileData.ftLastWriteTime;
		SYSTEMTIME st;
		FileTimeToSystemTime(&ftLastMod, &st);
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szFileDate, 255);
		if (write_out)
			ir_intf->writeStringEntry("File Info", "PatchDate", szFileDate);
		outputStream << "[File Info]\r\n";
		outputStream << "PatchDate=" << szFileDate << "\r\n\r\n";
		outputStream << "[Port]\r\n";
		UINT ini_port = (UINT)ir_intf->readIntegerEntry("Port", "Port");
		outputStream << "Port=" << ini_port << "\r\n\r\n";
	}

	outputStream << "[Memory Offsets]" << "\r\n";

	for (const auto& entry : kPrimaryOffsets)
	{
		QWORD mystart	  = (QWORD)ir_intf->readIntegerEntry(entry.iniSection, "Start", true);
		string mypattern = ir_intf->readEscapeStrings(entry.iniSection, "Pattern");
		string mymask	  = ir_intf->readStringEntry(entry.iniSection, "Mask", true);

		QWORD matchAddr = findEQAbsolutePointer(mystart, 0x900000, (PBYTE)mypattern.c_str(), (PCHAR)mymask.c_str());

		outputStream << entry.iniSection << "=0x" << std::hex << matchAddr;

		if (matchAddr == NULL)
		{
			outputStream << " #Not Found\r\n";
			continue;
		}

		if (matchAddr == net_intf->current_offset((int)entry.offsetType))
		{
			outputStream << " # Match\r\n";
			continue;
		}

		if (!write_out)
		{
			outputStream << " # Does not match ini file.\r\n";
			EnableWindow(GetDlgItem(hDlg, IDC_BUTTON2), TRUE);
			continue;
		}

		std::stringstream strm;
		strm << "0x" << std::hex << matchAddr;
		if (ir_intf->writeStringEntry("Memory Offsets", entry.iniSection, strm.str().c_str()))
		{
			reload = true;
			outputStream << " # Written to ini file\r\n";
		}
		else
		{
			outputStream << " # Found - Write failed\r\n";
		}
	}

	SetDlgItemText(hDlg, IDC_EDIT2, outputStream.str().c_str());

	return reload;
}

void EQGameScanner::ScanSecondary(HWND hDlg, IniReaderInterface* ir_intf, NetworkServerInterface* net_intf)
{
	if (!executableExists())
	{
		SetDlgItemText(hDlg, IDC_EDIT2, "Error: Could not locate the specified executable file.");
		return;
	}

	std::ostringstream findResults;
	std::ostringstream outputStream;

	WIN32_FILE_ATTRIBUTE_DATA FileData = {0};
	if (GetFileAttributesEx(executablePath.c_str(), GetFileExInfoStandard, &FileData))
	{
		TCHAR szFileDate[255];
		FILETIME ftLastMod = FileData.ftLastWriteTime;
		SYSTEMTIME st;
		FileTimeToSystemTime(&ftLastMod, &st);
		GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, szFileDate, 255);
		string::size_type index = executablePath.find_last_of("\\/");
		string myfilename		= executablePath.substr(index + 1, executablePath.size()).c_str();
		findResults << myfilename.c_str() << " Modified=" << szFileDate << "\r\n";
	}

	// Resolve CharInfo for this scan; a live pattern match is preferred,
	// falling back to whatever the ini currently has.
	EQPrimaryOffsets::CharInfo = net_intf->current_offset((int)NetworkServer::OT_self);
	{
		QWORD mystart	  = (QWORD)ir_intf->readIntegerEntry("CharInfo", "Start", true);
		string mypattern = ir_intf->readEscapeStrings("CharInfo", "Pattern");
		string mymask	  = ir_intf->readStringEntry("CharInfo", "Mask", true);

		QWORD matchAddr = findEQAbsolutePointer(mystart, 0x900000, (PBYTE)mypattern.c_str(), (PCHAR)mymask.c_str());
		if (matchAddr != NULL)
			EQPrimaryOffsets::CharInfo = matchAddr;
	}

	outputStream << "SpawnInfo Offsets" << "\r\n";

	for (const auto& entry : kSpawnInfoOffsets)
	{
		QWORD mystart	  = (QWORD)ir_intf->readIntegerEntry(entry.iniSection, "Start", true);
		string mypattern = ir_intf->readEscapeStrings(entry.iniSection, "Pattern");
		string mymask	  = ir_intf->readStringEntry(entry.iniSection, "Mask", true);

		QWORD matchAddr = findEQPointerOffset(mystart, 0x900000, (PBYTE)mypattern.c_str(), (PCHAR)mymask.c_str());

		outputStream << entry.displayName << ":" << "\r\n";
		outputStream << "| Match Found @ " << ((matchAddr == NULL) ? "FALSE" : "TRUE") << "\r\n";
		outputStream << "| Offset -> 0x" << std::hex << matchAddr << "\r\n";
		outputStream << "\r\n";
	}

	findResults << "\r\n";

	std::string v = findResults.str() + outputStream.str();
	SetDlgItemText(hDlg, IDC_EDIT2, v.c_str());
}
