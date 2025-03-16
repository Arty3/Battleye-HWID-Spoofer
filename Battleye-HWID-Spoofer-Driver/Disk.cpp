#include "Spoofer.h"
#include "decl.h"

/*
	Spoof disk drives (i.e. C:\),
	all disk drives are supported
*/

NTSTATUS Spoofer::DiskHandler(PDEVICE_OBJECT dev, PIRP irp)
{
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(irp);

	if (stack->Parameters.DeviceIoControl.InputBufferLength < 6)
		goto RET_ORIG_DISK;

	switch (stack->Parameters.DeviceIoControl.IoControlCode)
	{
	case 0x4D004:		/* IOCTL_SCSI_PASS_THROUGH					*/
	case 0x70000:		/* IOCTL_DISK_GET_DRIVE_LAYOUT				*/
	case 0x70050:		/* IOCTL_DISK_GET_DRIVE_GEOMETRY			*/
	case 0x70140:		/* IOCTL_DISK_GET_PARTITION_INFO			*/
	case 0x4D014:		/* IOCTL_SCSI_PASS_THROUGH_DIRECT			*/
	case 0x2D1400:		/* IOCTL_STORAGE_QUERY_PROPERTY				*/
	case 0x2D5100:		/* IOCTL_STORAGE_GET_HOTPLUG_INFO			*/
	case 0x2D1080:		/* IOCTL_STORAGE_GET_DEVICE_NUMBER			*/
	case 0x2D1440:		/* IOCTL_STORAGE_GET_MEDIA_TYPES_EX			*/
	case 0x2D0C14:		/* IOCTL_STORAGE_GET_MEDIA_SERIAL_NUMBER	*/

		irp->IoStatus.Information	= 0;
		irp->IoStatus.Status		= STATUS_DEVICE_NOT_CONNECTED;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_DEVICE_NOT_CONNECTED;

	case 0x74080:	/* SMART_GET_VERSION		*/
	case 0x740C8:	/* SMART_RCV_DRIVE_DATA		*/
	case 0x74084:	/* SMART_SEND_DRIVE_COMMAND	*/

		irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;

		IoCompleteRequest(irp, IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}

RET_ORIG_DISK:
	return Spoofer::og_disk(dev, irp);
}
