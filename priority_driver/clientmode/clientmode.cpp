#include <windows.h>
#include <stdio.h>
#include "..\HEADER.h"


int Error(const char* message);


int main(int argc, const char* argv[]) {
	if (argc < 3) {
		printf("Usage: Boost priority <threadid> <priority>\n");
		return 0;
	}
	int tid = atoi(argv[1]);
	int priority = atoi(argv[2]);
	HANDLE hDevice = CreateFile(L"\\\\.\\priority_driver", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDevice == INVALID_HANDLE_VALUE) {
		return Error("Failed to open Device");
	}

	ThreadData data;
	data.ThreadId = tid;
	data.Priority = priority;

	DWORD returned; 
	BOOL success = WriteFile(hDevice, &data, sizeof(data), &returned, nullptr);
	if (!success) {
		return Error("Priority change failed!");
	}
	printf("Priority change succeeded!\n");
	CloseHandle(hDevice);
}
int Error(const char* message) {
	printf("%s (error=%u)\n", message, GetLastError());
	return 1;
}
