#include "Spoofer.h"
#include "decl.h"

#define _NUM_HANDLERS 2

extern "C" NTSTATUS DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	Spoofer::NullifySMBIOS();

	UNICODE_STRING	drv_names[_NUM_HANDLERS];
	PDRIVER_OBJECT	drv_objcs[_NUM_HANDLERS];

	RtlZeroMemory(drv_objcs, sizeof(drv_objcs));

	/* Target driver names */
	RtlInitUnicodeString(&drv_names[0], L"\\Driver\\Disk");
	RtlInitUnicodeString(&drv_names[1], L"\\Driver\\TPM");

	for (int i = 0; i < _NUM_HANDLERS; ++i)
	{
		/* Lookup driver objects for target devices */
		NTSTATUS status = ObReferenceObjectByName(
			&drv_names[i], OBJ_CASE_INSENSITIVE,
			0, 0, *IoDriverObjectType, KernelMode,
			0, (PVOID*)&drv_objcs[i]
		);

		/* Skip device */
		if (!NT_SUCCESS(status))
			continue;

		/* Assign our own device irp handlers */
		switch (i)
		{
		case 0:
			Spoofer::og_disk = drv_objcs[i]->MajorFunction[IRP_MJ_DEVICE_CONTROL];
			drv_objcs[i]->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Spoofer::DiskHandler;
			break;

		case 1:
			Spoofer::og_tpm = drv_objcs[i]->MajorFunction[IRP_MJ_DEVICE_CONTROL];
			drv_objcs[i]->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Spoofer::TMPHandler;
			break;
		}

		ObDereferenceObject(drv_objcs[i]);
	}

	return STATUS_SUCCESS;
}
