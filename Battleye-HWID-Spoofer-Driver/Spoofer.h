#pragma once

#include <ntifs.h>

namespace Spoofer
{
	/* Original driver irp ctl dispatchers */
	static inline PDRIVER_DISPATCH og_disk	= nullptr;
	static inline PDRIVER_DISPATCH og_tpm	= nullptr;

	/* SMBIOS Spoofer */
	NTSTATUS NullifySMBIOS(void);

	/* Device Handlers */
	NTSTATUS TMPHandler(PDEVICE_OBJECT dev, PIRP irp);
	NTSTATUS DiskHandler(PDEVICE_OBJECT dev, PIRP irp);
}
