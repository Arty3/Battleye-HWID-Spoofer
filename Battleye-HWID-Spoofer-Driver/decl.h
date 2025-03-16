#pragma once

/* Forward declarations needed for the spoofer. */

#include <ntifs.h>
#include <ntddk.h>
#include <windef.h>
#include <ntimage.h>
#include <ntdddisk.h>
#include <ntddscsi.h>
#include <ntddndis.h>
#include <mountmgr.h>
#include <mountdev.h>
#include <classpnp.h>

#include <ata.h>
#include <scsi.h>

#pragma region _DECLARATIONS

typedef enum _SYSTEM_INFORMATION_CLASS
{
	SystemBasicInformation	= 0,
	SystemModuleInformation	= 11
}	SYSTEM_INFORMATION_CLASS;

typedef struct _SYSTEM_MODULE
{
	ULONG_PTR	Reserved[2];
	PVOID		Base;
	ULONG		Size;
	ULONG		Flags;
	USHORT		Index;
	USHORT		Unknown;
	USHORT		LoadCount;
	USHORT		ModuleNameOffset;
	CHAR		ImageName[256];
}	SYSTEM_MODULE, *PSYSTEM_MODULE;

typedef struct _SYSTEM_MODULE_INFORMATION
{
	ULONG_PTR		ulModuleCount;
	SYSTEM_MODULE	Modules[1];
}	SYSTEM_MODULE_INFORMATION,
	*PSYSTEM_MODULE_INFORMATION;

extern "C"
{
	NTSTATUS ZwQuerySystemInformation(
		SYSTEM_INFORMATION_CLASS	systemInformationClass,
		PVOID						systemInformation,
		ULONG						systemInformationLength,
		PULONG						returnLength
	);

	NTSTATUS ObReferenceObjectByName(
		PUNICODE_STRING	ObjectName,
		ULONG			Attributes,
		PACCESS_STATE	AccessState,
		ACCESS_MASK		DesiredAccess,
		POBJECT_TYPE	ObjectType,
		KPROCESSOR_MODE	AccessMode,
		PVOID			ParseContext,
		PVOID*			Object
	);

	extern POBJECT_TYPE* IoDriverObjectType;
}

typedef struct
{
	UINT8 Type;
	UINT8 Length;
	UINT8 Handle[2];
}	SMBIOS_HEADER;

#pragma endregion
