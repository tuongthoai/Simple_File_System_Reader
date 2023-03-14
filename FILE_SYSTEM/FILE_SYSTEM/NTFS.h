#pragma once
#include <iostream>
#include <Windows.h>

class NTFS {
private:

	LPCWSTR drive;
	PBYTE BPB; //từ 0B đọc 73bytes

	UINT32 bytes_per_sector;
	UINT32 sectors_per_cluster;
	UINT64 disk_sector;
	UINT64 MFT_begin_cluster;
	UINT32 MFT_entry_size;

	PBYTE MFT;
public:

	NTFS(LPCWSTR drive) {
		this->drive = drive;
		BPB = new BYTE[512];

		Read_Sector(BPB, 0, 512);

		bytes_per_sector = readInt(BPB, 0x0B, 2);
		sectors_per_cluster = readInt(BPB, 0x0D, 1);
		disk_sector = readUInt64(BPB, 0x28, 8);
		MFT_begin_cluster = readUInt64(BPB, 0x30, 8);

		BYTE entry_size = 0;
		memcpy(&entry_size, BPB + 0x40, 1);
		MFT_entry_size = (1 << ConvertTwosComplementByteToInteger(entry_size));

		
	}

	BYTE ConvertTwosComplementByteToInteger(BYTE rawValue)
	{
		// If a positive value, return it
		if ((rawValue & 0x80) == 0)
		{
			return rawValue;
		}

		// Otherwise perform the 2's complement math on the value
		return (BYTE)(~(rawValue - 0x01));
	}

	INT32 readInt(BYTE* sector, int offset, int number)
	{
		INT32 k = 0;
		memcpy(&k, sector + offset, number);
		return k;
	}

	INT64 readInt64(BYTE* sector, int offset, int number)
	{
		INT64 k = 0;
		memcpy(&k, sector + offset, number);
		return k;
	}

	UINT32 readUInt(BYTE* sector, int offset, int number)
	{
		UINT32 k = 0;
		memcpy(&k, sector + offset, number);
		return k;
	}

	UINT64 readUInt64(BYTE* sector, int offset, int number)
	{
		UINT64 k = 0;
		memcpy(&k, sector + offset, number);
		return k;
	}

	/*
	* this method return bytes read from file with starting point from readPoint
	*
	* buffer:     locations to store result data
	* readPoint:  offset of starting byte
	* size:       number of bytes to read
	*
	* return 1: Read success 0: otherwise
	*/
	int Read_Sector(PBYTE& buffer, int readPoint, int size)
	{
		int retCode = 0;
		DWORD bytesRead;
		HANDLE device = NULL;

		device = CreateFile(this->drive,               // Drive to open
			GENERIC_READ,                              // Access mode
			FILE_SHARE_READ | FILE_SHARE_WRITE,        // Share Mode
			NULL,                                      // Security Descriptor
			OPEN_EXISTING,                             // How to create
			0,                                         // File attributes
			NULL);                                     // Handle to template

		if (device == INVALID_HANDLE_VALUE) // Open Error
		{
			std::cout << "CreateFile : " << GetLastError() << std::endl;
			std::cout << std::endl;
			return 0;
		}

		SetFilePointer(device, readPoint, NULL, FILE_BEGIN);//Set a Point to Read

		if (!ReadFile(device, buffer, size, &bytesRead, NULL))
		{
			std::cout << "Read_File_Content : " << GetLastError() << std::endl;
			return -1;
		}
		else
		{
			// cout << "Success !!!" << endl;
			return bytesRead;
		}
	}

	void ReadMFT() {
		UINT32 firstSectorOfMFT = this->MFT_begin_cluster * this->sectors_per_cluster;
		MFT = new BYTE(this->MFT_entry_size);


	}
};
