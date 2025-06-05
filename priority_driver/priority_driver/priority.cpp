#include <ntifs.h>
#include <dontuse.h>

//prototype function
void BoosterUnload(PDRIVER_OBJECT DriverObject);
//prototype function for BoosterCreateClose, to approve handle requests
NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP irp); //irp = i/o request packet. 
//it's the primary object where the request information is stored, for all type of requests.

//***********************************************************************************************
//typically, you'd use DeviceIOControl here instead of WriteFile, 
//but WriteFile is easier to deal with right now, even though this isn't an actual Write-to-file operation.
NTSTATUS BoosterWrite(PDEVICE_OBJECT DeviceObject, PIRP irp);


//______________________________________________________________________________________________________________________________________________________________
//The following 2 functions are used as reference, from Pavel Yosifovich's Windows Kernel Programming.



/*NTSTATUS IoCreateDevice(
	_In_ PDRIVER_OBJECT DriverObject, //the driver object to which this device object belongs to. 
	_In_ ULONG DeviceExtensionSize,  //extra bytes that are to be allocated (in addition to sizeof(DEVICE_OBJECT). 
	_In_opt_ PUNICODE_STRING DeviceName, //internal device name
	_In_ DEVICE_TYPE DeviceType, //relevant to some type of hardware-based drivers. For software drivers, DeviceType = FILE_DEVICE_UNKNOWN.
	_In_ ULONG DeviceCharacteristics, //Set of flags relevant for some drivers. 
	_In_ BOOLEAN Exclusive, //Exclusive means only having 1 instance/client at a time fo rthe device.
	_Outptr_ PDEVICE_OBJECT* DeviceObject); //Returned pointer, passed as an address of a pointer. If STATUS_SUCCESS, IoCreateDevice allocated the structure from non-paged pool
											// and stores the resulting pointer inside the dereferenced argument.


NTSTATUS IoCreateSymbolicLink(_In_ PUNICODE_STRING SymbolicLinkName, _In_ PUNICODE_STRING DeviceName);// self explanatory. function that creates creates symbolic link.


*/


//***************************************************************************************** START OF DRIVER *********************************************************************************************************************************

extern "C" NTSTATUS 
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) { //always contains these 4: Set unload routine. Set dispatch routines the driver support, 
	//Create device Object, 
	//Create a symbolic link to the device object.
	
	//All drivers need to support IRP_MJ_Create and IRP_MJ_Close, or no handles can be made. 

	PDEVICE_OBJECT DeviceObject;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = BoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = BoosterCreateClose;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = BoosterWrite;
	UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\priority_driver");
	NTSTATUS status = IoCreateDevice(
		DriverObject, // our driver object
		0, // no need for extra bytes.
		&devName, // device name
		FILE_DEVICE_UNKNOWN, //device type
		0, //we dont have any characteristic flags
		FALSE, //not exclusive, can have many "instances"/clients
		&DeviceObject); //resulting pointer

	//run IoCreateDevice. if status != 0, run kdprint maco and print the error.
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device Object (0x%08X)\n", status));
		return status;
	}
	//run IoCreateSymbolicLink. if status != 0, run kdprint macro and print the error.
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\priority_driver");
	status = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(status)) {
		KdPrint(("Failed to create device object (0x%08X)\n", status));
		IoDeleteDevice(DeviceObject); //clears memory allocated from IoCreateDevice. ***VERY IMPORTANT!
			return status;
	}

	
}
void BoosterUnload(PDRIVER_OBJECT DriverObject) {
	//UNLOAD DRIVER
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\??\\priority_driver"); //get the symbolic link address & place in pointer
	IoDeleteSymbolicLink(&symLink); //delete the symbolic link
	IoDeleteDevice(DriverObject->DeviceObject); //deletes the device object.

}

NTSTATUS BoosterCreateClose(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	irp->IoStatus.Status = STATUS_SUCCESS; //indicates the status this request should complete with: Expected status.

	irp->IoStatus.Information = 0; // polymorphic member; which means different things in different request types. in the case of create and close, a value of 0 is fine.
	IoCompleteRequest(irp, IO_NO_INCREMENT);//completes IRP. propagates the IRP back to its creator, which notifies the client that the operation has completed and frees the IRP.
	//IO_NO_INCREMENT (the value in the second argument) is a temporary priority boost value that a driver can provide to its client. A value of 0 is fine in our case.
	return STATUS_SUCCESS; // we return the expected/same status as the one put into the IRP.
	//IMPORTANT!: Don't write "return irp->IoStatus.Status;", as that code is buggy. It will cause a BSOD in most cases, as after running IoCompleteRequest, the irp is terminated- so poisonous.
}

NTSTATUS BoosterWrite(PDEVICE_OBJECT DeviceObject, PIRP irp)
{
	//we must verify that the information sent in the irp is correct. we do so by checking the buffer size, to make sure it's at least the size we expect.

	NTSTATUS status = STATUS_SUCCESS;
	ULONG_PTR information = 0; 
	auto irpSp = IoGetCurrentIrpStackLocation(irp); //gets the stack location.
	do {
		if (irpSp->Parameters.Write.Length < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		auto data = static_cast<ThreadData*>(irp->UserBuffer); //user buffer is a void pointer, so we need to cast it to the expected type. then we check the priority value, 
		//and if not in range, change the status to invalid paramter and break out of the do-while(false) loop.
		//**IMPORTANT: The use of static_cast asks the compiler to check if the cast makes sense. it's a good habit to have, since it catches bugs that may be overlooked.
		if (data == nullptr || data->Priority < 1 || data->Priority>31) { //we compare the pointer to null first, as no further checks are made if false. this is due to
																		  //short circuit elevation.	
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		PETHREAD thread; 
		status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &thread);
		if (!NT_SUCCESS(status)) {
			break;
		}
		auto oldprio = KeSetPriorityThread(thread, data->Priority);
		KdPrint(("Priority change for thread %u from %d to %d succeeded!\n", data->ThreadId, oldprio, data->Priority)); //output the old thread for debugging purposes.

		ObDereferenceObject(thread); //after changing the prio of the thread, we must decrement the object's reference, or it's a leak. we're done with it, so we clean it up.
		information = sizeof(data);
		//we will write that value to the IRP before completing it. it's what's returned as the second to last argument from the client's WritewFile call.
	} while (false);

	//closing the irp
	irp->IoStatus.Status = status;
	irp->IoStatus.Information = information;
	IoCompleteRequest(irp, IO_NO_INCREMENT);


	
	
	return status;
}

struct ThreadData {
	ULONG ThreadId; //we cant use DWORD here, as it's not defined in kernel mode headers. ULONG is, and it's pretty much the same thing .
	int Priority;
};


