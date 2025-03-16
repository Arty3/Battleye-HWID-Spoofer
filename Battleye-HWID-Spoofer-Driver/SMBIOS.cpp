#include "Spoofer.h"
#include "decl.h"

#define _ALLOC_TAG 'x916'

/* https://www.dmtf.org/standards/smbios */

static __forceinline BOOLEAN LocateSMBIOS(const char* __restrict data, const ULONG k)
{
	/*
		Pattern scans for the following byte sequence in x86-64 asm:

		MOV RCX, [RIP + offset]		; Load a global pointer
		TEST RCX, RCX				; Check if it's NULL
		JZ location					; Jump if NULL
		MOV EDX, [RIP + offset]		; Load value

		This retrieves the SMBIOS address
	*/

	return	data[k + 0]		== '\x48' && data[k + 1]	== '\x8B' && data[k + 2]	== '\x0D' &&
			data[k + 7]		== '\x48' && data[k + 8]	== '\x85' && data[k + 9]	== '\xC9' &&
			data[k + 10]	== '\x74' && data[k + 12]	== '\x8B' && data[k + 13]	== '\x15';
}

NTSTATUS Spoofer::NullifySMBIOS(void)
{
	NTSTATUS	status;
	ULONG		size;

	/* First call gets buffer size */
	status = ZwQuerySystemInformation(
		SystemModuleInformation, nullptr, 0, &size
	);

	if (!NT_SUCCESS(status))
		return status;

	auto list = static_cast<PSYSTEM_MODULE_INFORMATION>(
		ExAllocatePool2(POOL_FLAG_NON_PAGED, size, _ALLOC_TAG)
	);

	if (!list)
		return STATUS_INSUFFICIENT_RESOURCES;

	/* Second call gets the actual data */
	status = ZwQuerySystemInformation(
		SystemModuleInformation, list, size, nullptr
	);

	if (!NT_SUCCESS(status))
	{
		ExFreePoolWithTag(list, _ALLOC_TAG);
		return status;
	}

	for (ULONG i = 0; i < list->ulModuleCount; ++i)
	{
		/* Search for the NT Kernel image */
		if (!strstr(list->Modules[i].ImageName, "ntoskrnl.exe"))
			continue;

		PVOID base = list->Modules[i].Base;

		/* Get the NT Kernel image headers */
		auto headers = reinterpret_cast<PIMAGE_NT_HEADERS>(
			static_cast<char*>(base) +
			static_cast<PIMAGE_DOS_HEADER>(base)->e_lfanew
		);

		PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(headers);

		for (ULONG j = 0; j < headers->FileHeader.NumberOfSections; ++j)
		{
			/* Find little endian PAGE sections or executable code section (.text) */
			if ('EGAP' != *reinterpret_cast<PINT>(sections[j].Name) &&
				memcmp(sections[j].Name, ".text", 5)) goto FREE_AND_RETURN;

			/* Pattern scan for SMBIOS table ptr */
			char* data = static_cast<char*>(base) + sections[j].VirtualAddress;
			
			/* Avoid any underflow misshaps */
			if (sections[j].Misc.VirtualSize < 14)
				continue;

			PPHYSICAL_ADDRESS addr = nullptr;

			/* Search for SMBIOS in image header sections */
			for (ULONG k = 0; k <= sections[j].Misc.VirtualSize - 14; ++k)
			{
				if (!LocateSMBIOS(data, k))
					continue;

				/* Convert SMBIOS address from relative to physical */
				addr = reinterpret_cast<PPHYSICAL_ADDRESS>(
					&data[k] + 7 + *reinterpret_cast<int*>(&data[k] + 3)
				);
			}

			if (!addr)
				goto FREE_AND_RETURN;

			/* Map the SMBIOS address into memory */
			PVOID mapped = MmMapIoSpace(*addr, 0x1000, MmNonCached);

			if (!mapped)
				goto FREE_AND_RETURN;


			/*
				Standard SMBIOS Table Entry:

				+----------------------+  <- entry (SMBIOS_HEADER)
				| Fixed-size header    |
				+----------------------+
				| String 1 ("ASUS")    |  <- Null-terminated
				| \0                   |
				| String 2 ("ROG")     |  <- Null-terminated
				| \0                   |
				| String 3 ("123456")  |  <- Null-terminated
				| \0                   |
				| \0  <- Double null   |  <- End of entry
				+----------------------+
				| Next SMBIOS entry    |  <- New entry starts here
				+----------------------+
			*/

			SMBIOS_HEADER* entry = static_cast<SMBIOS_HEADER*>(mapped);

			/* Iterate over each SMBIOS entry */
			for ( ; entry->Type != 127; )
			{
				/* Get string area */
				char* s = reinterpret_cast<char*>(entry) + entry->Length;

				/* Length of string area */
				size_t len = 0;
				do
					len += strlen(s) + 1;
				while (*(s + len));

				if (entry->Type == 1 || entry->Type == 2)
				{
					/*
						Zeroes out the SMBIOS Type 1 and Type 2 entries
						Type 1 being system info (product name, serial num, etc.)
						Type 2 being baseboard (motherboard) information.
					*/
					RtlZeroMemory(
						reinterpret_cast<char*>(entry) + sizeof(SMBIOS_HEADER),
						entry->Length - sizeof(SMBIOS_HEADER)
					);

					RtlZeroMemory(s, len);
				}

				/* SMBIOS tables end with `\0\0`, this is already accounted for by len */
				entry = reinterpret_cast<SMBIOS_HEADER*>(s + entry->Length + len);
			}

			MmUnmapIoSpace(mapped, 0x1000);
			ExFreePoolWithTag(list, _ALLOC_TAG);

			return STATUS_SUCCESS;
		}
	}

FREE_AND_RETURN:
	ExFreePoolWithTag(list, _ALLOC_TAG);
	return STATUS_UNSUCCESSFUL;
}
