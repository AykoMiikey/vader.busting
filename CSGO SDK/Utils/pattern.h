#pragma once

#include "address.h"
#include "stack.h"
#include "../SDK/sdk.hpp"
#include "modules.h"

#define INRANGE(x,a,b)	(x >= a && x <= b) 
#define GetBits(x)		(INRANGE((x&(~0x20)),'A','F') ? ((x&(~0x20)) - 'A' + 0xa) : (INRANGE(x,'0','9') ? x - '0' : 0))
#define GetByte(x)		(GetBits(x[0]) << 4 | GetBits(x[1]))

namespace pattern {
	// container
	using patterns_t = std::vector< Address >;
	using pattern_byte_t = std::pair< uint8_t, bool >;

	static Address find(Address start, size_t len, const std::string& pat) {
		uint8_t* scan_start, * scan_end;
		std::vector< pattern_byte_t > pattern{};
		std::stringstream			  stream{ pat };
		std::string				      w;

		if (!start || !len || pat.empty())
			return{};

		// split spaces and convert to hex.
		while (stream >> w) {
			// wildcard.
			if (w[0] == '?')
				pattern.push_back({ 0, true });

			// valid hex digits.
			else if (std::isxdigit(w[0]) && std::isxdigit(w[1]))
				pattern.push_back({ (uint8_t)std::strtoul(w.data(), 0, 16), false });
		}

		scan_start = start.as< uint8_t* >();
		scan_end = scan_start + len;

		// find match.
		auto result = std::search(scan_start, scan_end, pattern.begin(), pattern.end(),
			[](const uint8_t b, const pattern_byte_t& p) {
				// byte matches or it's a wildcard.
				return b == p.first || p.second;
			});

		// nothing found.
		if (result == scan_end)
			return{};

		return (uintptr_t)result;
	}

	inline DWORD find_pattern(DWORD dwAddress, DWORD dwLength, PCCH szPattern)
	{
		PCCH szPat = szPattern;
		DWORD dwFirstMatch = 0x0;

		for (DWORD pCur = dwAddress; pCur < dwLength; pCur++)
		{
			if (!*szPat)
				return dwFirstMatch;

			if (*(PBYTE)szPat == '\?' || *(BYTE*)pCur == GetByte(szPat))
			{
				if (!dwFirstMatch)
					dwFirstMatch = pCur;

				if (!szPat[2])
					return dwFirstMatch;

				if (*(PWORD)szPat == '\?\?' || *(PBYTE)szPat != '\?')
					szPat += 3;

				else szPat += 2;
			}

			else {
				szPat = szPattern;
				dwFirstMatch = 0;
			}
		}

		return 0x0;
	}

	inline HMODULE get_module_handle(PCCH szModuleName)
	{
		HMODULE hmModuleHandle = 0;

		do
		{
			hmModuleHandle = GetModuleHandleA(szModuleName);
			Sleep(1);
		} while (hmModuleHandle == 0);

		return hmModuleHandle;
	}

	inline DWORD find_sig(PCCH szModuleName, PCCH szPattern)
	{
		HMODULE hmModule = get_module_handle(szModuleName);
		PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hmModule;
		PIMAGE_NT_HEADERS pNTHeaders = (PIMAGE_NT_HEADERS)(((DWORD)hmModule) + pDOSHeader->e_lfanew);

		return find_pattern(((DWORD)hmModule) + pNTHeaders->OptionalHeader.BaseOfCode, ((DWORD)hmModule) + pNTHeaders->OptionalHeader.SizeOfCode, szPattern);
	}



	static Address find(const PE::Module& module, const std::string& pat) {
		return find(module.GetBase(), module.GetImageSize(), pat);
	}
	static patterns_t FindAll(Address start, size_t len, const std::string& pat) {
		patterns_t out{};
		Address	   result;

		for (;; ) {
			// find result.
			result = find(start, len, pat);
			if (!result)
				break;

			// if we arrived here we found something.
			out.push_back(result);

			// set new len.
			len = (start + len) - (result + 1);

			// new start point.
			start = result + 1;
		}

		return out;
	}

	static patterns_t FindAll(const PE::Module& module, const std::string& pat) {
		return FindAll(module.GetBase(), module.GetImageSize(), pat);
	}
}