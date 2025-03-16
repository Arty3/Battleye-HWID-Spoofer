#include "Spoofer.h"

/*
	TPM (Trusted Platform Module) is a physical
	chip that securely stores artifacts used to
	authenticate the platform (your PC).

	It's used to store things such as passwords,
	encryption keys, certificates and more.

	For our purposes we're going to modify the
	TMP driver's state to register it as disabled.

	https://trustedcomputinggroup.org/wp-content/uploads/Trusted-Platform-Module-Summary_04292008.pdf
*/

NTSTATUS Spoofer::TMPHandler(PDEVICE_OBJECT dev, PIRP irp)
{
	switch (IoGetCurrentIrpStackLocation(irp)
		->Parameters.DeviceIoControl.IoControlCode)
	{
	case 0x22C194:	/* TMP_GET_INFO			*/
	case 0x22C004:	/* TMP_READ_DATA		*/
	case 0x22C010:	/* TMP_GET_VERSION		*/
	case 0x22C01C:	/* TMP_SEND_COMMAND		*/
	case 0x22C014:	/* TMP_GET_CAPABILITY	*/
	case 0x22C00C:	/* TMP_EXECUTE_COMMAND	*/

		irp->IoStatus.Information	= 0;
		irp->IoStatus.Status		= STATUS_DEVICE_NOT_READY;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_DEVICE_NOT_READY;
	}

	return Spoofer::og_tpm(dev, irp);
}
