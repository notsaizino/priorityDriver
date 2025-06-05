
#include <windows.h>


//prototype function
void BoosterUnload(PDRIVER_OBJECT DriverObject);
//prototype function for BoosterCreateClose, to approve handle requests
NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP irp); //irp = i/o request packet. 
//it's the primary object where the request information is stored, for all type of requests.

//***********************************************************************************************
//typically, you'd use DeviceIOControl here instead of WriteFile, 
//but WriteFile is easier to deal with right now, even though this isn't an actual Write-to-file operation.

NTSTATUS BoosterWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);


struct ThreadData {
	ULONG ThreadId; 
	int Priority;
};
