/*
 * Offline smoke tests for the byte-pattern-scanning core of this tool:
 * EQGameScanner::findEQPointerOffset / findEQAbsolutePointer (which
 * findEQAbsolutePointer's PE-header handling and compareData's x/?/t
 * matching all run through), plus the IniReader::readEscapeStrings
 * decoder that turns a config.ini "Pattern=\x01\x48..." string into the
 * raw bytes those functions are handed.
 *
 * No real eqgame.exe is needed: each test builds a small synthetic PE32+
 * file (or a plain ini file) on disk, points the real production classes
 * at it, and checks the result - this is the same pipeline used against a
 * real client, just fed a byte layout we control so the expected answer
 * is known ahead of time. See improvement-suggestions #14 (this repo's
 * private tracking doc) for why this exists: everything here is pure and
 * offline-testable, and it's the actual mechanism that lets this tool
 * work at all against a given EQ client build.
 */
#include "stdafx.h"

#include "EQGameScanner.h"
#include "IniReader.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int g_checks	= 0;
int g_failures	= 0;

#define CHECK_EQ(actual, expected)                                                                        \
	do                                                                                                     \
	{                                                                                                      \
		++g_checks;                                                                                       \
		QWORD _a = (QWORD)(actual);                                                                       \
		QWORD _e = (QWORD)(expected);                                                                     \
		if (_a != _e)                                                                                     \
		{                                                                                                 \
			++g_failures;                                                                                 \
			std::cout << "FAIL " << __LINE__ << ": " << #actual << " == " << #expected << " (got 0x"      \
					  << std::hex << _a << ", expected 0x" << _e << std::dec << ")" << std::endl;          \
		}                                                                                                  \
	} while (0)

#define CHECK(cond)                                                                                       \
	do                                                                                                     \
	{                                                                                                      \
		++g_checks;                                                                                       \
		if (!(cond))                                                                                      \
		{                                                                                                 \
			++g_failures;                                                                                 \
			std::cout << "FAIL " << __LINE__ << ": " << #cond << std::endl;                                \
		}                                                                                                  \
	} while (0)

std::string MakeTempFilePath(const char* suffix)
{
	char tempDir[MAX_PATH];
	GetTempPathA(MAX_PATH, tempDir);
	char tempFile[MAX_PATH];
	GetTempFileNameA(tempDir, "eqt", 0, tempFile);
	return std::string(tempFile) + suffix;
}

void WriteBinaryFile(const std::string& path, const std::vector<BYTE>& bytes)
{
	std::ofstream file(path.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
	file.write((const char*)bytes.data(), (std::streamsize)bytes.size());
}

void WriteTextFile(const std::string& path, const std::string& text)
{
	std::ofstream file(path.c_str(), std::ios::out | std::ios::trunc);
	file << text;
}

// Builds a minimal-but-valid (for EQGameScanner::loadPEInfo's purposes)
// PE32+ image: DOS/PE/COFF/optional headers plus a single section, with
// `patches` copied into the buffer at the given absolute file offsets -
// this is where callers embed the synthetic byte patterns under test.
std::vector<BYTE> BuildSyntheticPE(QWORD imageBase, QWORD sectionRVA, QWORD sectionVSize, QWORD sectionFileOffset,
	QWORD sectionFileSize, const std::vector<std::pair<QWORD, std::vector<BYTE>>>& patches)
{
	std::vector<BYTE> buf((size_t)(sectionFileOffset + sectionFileSize), 0);

	buf[0] = 'M';
	buf[1] = 'Z';
	DWORD e_lfanew = 0x80;
	memcpy(&buf[0x3C], &e_lfanew, sizeof(e_lfanew));

	size_t pos	= e_lfanew;
	buf[pos + 0] = 'P';
	buf[pos + 1] = 'E';
	buf[pos + 2] = 0;
	buf[pos + 3] = 0;
	pos += 4;

	size_t coffStart		  = pos;
	WORD numSections		  = 1;
	WORD sizeOfOptionalHeader = 32; // covers magic(2) .. ImageBase(8 @ +24), ends at +32
	memcpy(&buf[coffStart + 2], &numSections, sizeof(numSections));
	memcpy(&buf[coffStart + 16], &sizeOfOptionalHeader, sizeof(sizeOfOptionalHeader));
	pos = coffStart + 20;

	size_t optHeaderStart = pos;
	WORD magic			   = 0x20B; // PE32+
	memcpy(&buf[optHeaderStart], &magic, sizeof(magic));
	memcpy(&buf[optHeaderStart + 24], &imageBase, sizeof(imageBase));

	pos = optHeaderStart + sizeOfOptionalHeader;

	DWORD vSize = (DWORD)sectionVSize;
	DWORD vAddr = (DWORD)sectionRVA;
	DWORD rSize = (DWORD)sectionFileSize;
	DWORD rPtr	= (DWORD)sectionFileOffset;
	memcpy(&buf[pos + 8], &vSize, sizeof(vSize));
	memcpy(&buf[pos + 12], &vAddr, sizeof(vAddr));
	memcpy(&buf[pos + 16], &rSize, sizeof(rSize));
	memcpy(&buf[pos + 20], &rPtr, sizeof(rPtr));

	for (const auto& patch : patches)
	{
		std::copy(patch.second.begin(), patch.second.end(), buf.begin() + (ptrdiff_t)patch.first);
	}

	return buf;
}

const QWORD kImageBase	  = 0x140000000ULL;
const QWORD kSectionRVA	  = 0x1000;
const QWORD kSectionVSize = 0x2000;
const QWORD kSectionStart = 0x400; // matches this repo's own config.ini convention
const QWORD kSectionSize  = 0x2000;
const std::size_t kBlockSize = 0x2000;

void TestFindEQPointerOffset_WildcardAndByteCapture(EQGameScanner& scanner)
{
	// x?xt: byte0 exact, byte1 wildcarded, byte2 exact, byte3 captured (typelen=1).
	std::vector<std::pair<QWORD, std::vector<BYTE>>> patches = {
		{ 0x420, { 0xAA, 0x77, 0xBB, 0x2A } },
	};
	auto pe = BuildSyntheticPE(kImageBase, kSectionRVA, kSectionVSize, kSectionStart, kSectionSize, patches);
	auto path = MakeTempFilePath(".exe");
	WriteBinaryFile(path, pe);

	BYTE byteMask[] = { 0xAA, 0x00, 0xBB, 0x2A };
	char charMask[] = "x?xt";
	scanner.setExe((TCHAR*)path.c_str());
	QWORD result = scanner.findEQPointerOffset(kSectionStart, kBlockSize, byteMask, charMask);
	CHECK_EQ(result, 0x2A);

	DeleteFileA(path.c_str());
}

void TestFindEQPointerOffset_DWordCapture(EQGameScanner& scanner)
{
	std::vector<std::pair<QWORD, std::vector<BYTE>>> patches = {
		{ 0x430, { 0x11, 0x22, 0x33, 0x44, 0x78, 0x56, 0x34, 0x12, 0x99 } },
	};
	auto pe	  = BuildSyntheticPE(kImageBase, kSectionRVA, kSectionVSize, kSectionStart, kSectionSize, patches);
	auto path = MakeTempFilePath(".exe");
	WriteBinaryFile(path, pe);

	BYTE byteMask[] = { 0x11, 0x22, 0x33, 0x44, 0x00, 0x00, 0x00, 0x00, 0x99 };
	char charMask[] = "xxxxttttx";
	scanner.setExe((TCHAR*)path.c_str());
	QWORD result = scanner.findEQPointerOffset(kSectionStart, kBlockSize, byteMask, charMask);
	CHECK_EQ(result, 0x12345678);

	DeleteFileA(path.c_str());
}

void TestFindEQPointerOffset_NoMatchReturnsZero(EQGameScanner& scanner)
{
	std::vector<std::pair<QWORD, std::vector<BYTE>>> patches; // nothing embedded
	auto pe	  = BuildSyntheticPE(kImageBase, kSectionRVA, kSectionVSize, kSectionStart, kSectionSize, patches);
	auto path = MakeTempFilePath(".exe");
	WriteBinaryFile(path, pe);

	BYTE byteMask[] = { 0xDE, 0xAD, 0xBE, 0xEF };
	char charMask[] = "xxxx";
	scanner.setExe((TCHAR*)path.c_str());
	QWORD result = scanner.findEQPointerOffset(kSectionStart, kBlockSize, byteMask, charMask);
	CHECK_EQ(result, 0);

	DeleteFileA(path.c_str());
}

void TestFindEQPointerOffset_ImplausibleValueIsRejected(EQGameScanner& scanner)
{
	// The byte pattern matches, but the captured DWORD (0x30000000) is
	// >= 536870912, so findEQPointerOffset's plausibility filter should
	// discard this match; since it's the only candidate, the result is 0.
	std::vector<std::pair<QWORD, std::vector<BYTE>>> patches = {
		{ 0x440, { 0x55, 0x66, 0x00, 0x00, 0x00, 0x30, 0x77 } },
	};
	auto pe	  = BuildSyntheticPE(kImageBase, kSectionRVA, kSectionVSize, kSectionStart, kSectionSize, patches);
	auto path = MakeTempFilePath(".exe");
	WriteBinaryFile(path, pe);

	BYTE byteMask[] = { 0x55, 0x66, 0x00, 0x00, 0x00, 0x00, 0x77 };
	char charMask[] = "xxttttx";
	scanner.setExe((TCHAR*)path.c_str());
	QWORD result = scanner.findEQPointerOffset(kSectionStart, kBlockSize, byteMask, charMask);
	CHECK_EQ(result, 0);

	DeleteFileA(path.c_str());
}

void TestFindEQAbsolutePointer_ResolvesRipRelative(EQGameScanner& scanner)
{
	// Same shape as config.ini's real ZoneAddr pattern: lea reg,[rip+disp32]
	// immediately followed by another instruction. disp32=0x500 is chosen
	// so the resolved address lands inside the synthetic section.
	std::vector<std::pair<QWORD, std::vector<BYTE>>> patches = {
		{ 0x410, { 0x01, 0x48, 0x89, 0x1D, 0x00, 0x05, 0x00, 0x00, 0x48, 0x8B, 0x01, 0xFF } },
	};
	auto pe	  = BuildSyntheticPE(kImageBase, kSectionRVA, kSectionVSize, kSectionStart, kSectionSize, patches);
	auto path = MakeTempFilePath(".exe");
	WriteBinaryFile(path, pe);

	BYTE byteMask[] = { 0x01, 0x48, 0x89, 0x1D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x01, 0xFF };
	char charMask[] = "xxxxttttxxxx";
	scanner.setExe((TCHAR*)path.c_str());
	QWORD result = scanner.findEQAbsolutePointer(kSectionStart, kBlockSize, byteMask, charMask);
	CHECK_EQ(result, kImageBase + 0x1518);

	DeleteFileA(path.c_str());
}

void TestFindEQAbsolutePointer_RejectsResolutionOutsideMappedSection(EQGameScanner& scanner)
{
	// Structurally matches, but its disp32 (0x500000) resolves far past
	// the synthetic section's end - this must be rejected as a likely
	// coincidental byte match rather than a genuine RIP-relative reference.
	std::vector<std::pair<QWORD, std::vector<BYTE>>> patches = {
		{ 0x450, { 0x02, 0x49, 0x8A, 0x1E, 0x00, 0x00, 0x50, 0x00, 0x49, 0x8C, 0x02, 0xFE } },
	};
	auto pe	  = BuildSyntheticPE(kImageBase, kSectionRVA, kSectionVSize, kSectionStart, kSectionSize, patches);
	auto path = MakeTempFilePath(".exe");
	WriteBinaryFile(path, pe);

	BYTE byteMask[] = { 0x02, 0x49, 0x8A, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x49, 0x8C, 0x02, 0xFE };
	char charMask[] = "xxxxttttxxxx";
	scanner.setExe((TCHAR*)path.c_str());
	QWORD result = scanner.findEQAbsolutePointer(kSectionStart, kBlockSize, byteMask, charMask);
	CHECK_EQ(result, 0);

	DeleteFileA(path.c_str());
}

void TestFindEQAbsolutePointer_NonPEFileReturnsZero(EQGameScanner& scanner)
{
	std::vector<BYTE> notAPE(0x100, 0);
	notAPE[0] = 'X';
	notAPE[1] = 'X'; // deliberately not "MZ"
	auto path = MakeTempFilePath(".exe");
	WriteBinaryFile(path, notAPE);

	BYTE byteMask[] = { 0x01, 0x02, 0x03, 0x04 };
	char charMask[] = "xxtt";
	scanner.setExe((TCHAR*)path.c_str());
	QWORD result = scanner.findEQAbsolutePointer(0, 0x100, byteMask, charMask);
	CHECK_EQ(result, 0);

	DeleteFileA(path.c_str());
}

void TestReadEscapeStrings_DecodesRealPatternShape(IniReader& ini)
{
	auto path = MakeTempFilePath(".ini");
	WriteTextFile(path,
		"[TestZoneAddr]\r\n"
		"Pattern=\\x01\\x48\\x89\\x1D\\x00\\x05\\x00\\x00\\x48\\x8B\\x01\\xFF\r\n"
		"Mask=xxxxttttxxxx\r\n");

	ini.openConfigFile(path);
	std::string decoded = ini.readEscapeStrings("TestZoneAddr", "Pattern");
	std::string mask	 = ini.readStringEntry("TestZoneAddr", "Mask", true);

	const BYTE expected[] = { 0x01, 0x48, 0x89, 0x1D, 0x00, 0x05, 0x00, 0x00, 0x48, 0x8B, 0x01, 0xFF };
	CHECK_EQ(decoded.size(), sizeof(expected));
	bool bytesMatch = decoded.size() == sizeof(expected) &&
		memcmp(decoded.data(), expected, sizeof(expected)) == 0;
	CHECK(bytesMatch);
	CHECK(mask == "xxxxttttxxxx");

	DeleteFileA(path.c_str());
}

void TestReadEscapeStrings_MalformedEscapeIsSkippedNotFatal(IniReader& ini)
{
	// \q isn't a valid \x escape (should be silently dropped), and the
	// bare "XY" that follows isn't part of any escape sequence either -
	// decoding should just resume cleanly with the next \xNN.
	auto path = MakeTempFilePath(".ini");
	WriteTextFile(path,
		"[TestMalformed]\r\n"
		"Pattern=\\xAB\\xCd\\x9F\\qXY\\x00\r\n");

	ini.openConfigFile(path);
	std::string decoded = ini.readEscapeStrings("TestMalformed", "Pattern");

	const BYTE expected[] = { 0xAB, 0xCD, 0x9F, 0x00 };
	CHECK_EQ(decoded.size(), sizeof(expected));
	bool bytesMatch = decoded.size() == sizeof(expected) &&
		memcmp(decoded.data(), expected, sizeof(expected)) == 0;
	CHECK(bytesMatch);

	DeleteFileA(path.c_str());
}

} // namespace

int main()
{
	EQGameScanner scanner;
	IniReader ini;

	TestFindEQPointerOffset_WildcardAndByteCapture(scanner);
	TestFindEQPointerOffset_DWordCapture(scanner);
	TestFindEQPointerOffset_NoMatchReturnsZero(scanner);
	TestFindEQPointerOffset_ImplausibleValueIsRejected(scanner);
	TestFindEQAbsolutePointer_ResolvesRipRelative(scanner);
	TestFindEQAbsolutePointer_RejectsResolutionOutsideMappedSection(scanner);
	TestFindEQAbsolutePointer_NonPEFileReturnsZero(scanner);
	TestReadEscapeStrings_DecodesRealPatternShape(ini);
	TestReadEscapeStrings_MalformedEscapeIsSkippedNotFatal(ini);

	std::cout << (g_checks - g_failures) << "/" << g_checks << " checks passed" << std::endl;
	return g_failures == 0 ? 0 : 1;
}
