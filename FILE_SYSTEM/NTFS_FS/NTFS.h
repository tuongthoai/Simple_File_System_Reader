#pragma once
#include <iostream>
#include <Windows.h>
#include "File.h"
#include "Folder.h"
#include "Abstract_File.h"

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
	Abstract_File* root = nullptr;
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

	std::string Get_String(BYTE* DATA, int offset, int size)
	{
		char* tmp = new char[size + 1];
		memcpy(tmp, DATA + offset, size);
		std::string s = "";
		for (int i = 0; i < size; i++)
			if (tmp[i] != 0x00 && tmp[i] != 0xFF)
				s += tmp[i];
		delete tmp;
		return s;
	}

	std::string toBinary(INT64 n)
	{
		std::string r;
		while (n != 0)
		{
			r = (n % 2 == 0 ? "0" : "1") + r;
			n /= 2;
		}
		return r;
	}

	std::wstring Get_UniCode_String(BYTE* DATA, int offset, int size)
	{
		char tmp[100];
		memcpy(tmp, DATA + offset, size);
		for (int i = 0; i < 100; i += 2) {
			std::swap(tmp[i], tmp[i + 1]);
		}
		int sz = MultiByteToWideChar(CP_UTF8, 0, &tmp[0], -1, 0, 0);
		std::wstring res(sz, 0);
		MultiByteToWideChar(CP_UTF8, 0, &tmp[0], -1, &res[0], sz + 1);
		return res;
	}

	/*
	* This method reads an MFT entry and the attributes inside the entry
	*	Note: We only pay attention 
	*/

	void ReadMFT() {
		UINT32 firstSectorOfMFT = this->MFT_begin_cluster * this->sectors_per_cluster;
		MFT = new BYTE(this->MFT_entry_size);
		this->Read_Sector(MFT, firstSectorOfMFT, this->MFT_entry_size);

		std::string Signature = Get_String(MFT, 0x00, 4);
		INT64 offsetToFirstAttr = readInt64(MFT, 0x14, 2);
		INT64 realSizeOfRecord = readInt64(MFT, 0x18, 4);

		INT64 pointer = offsetToFirstAttr;

		do {
			INT64 attrType = readInt64(MFT, pointer, 4);
			INT64 totalAttrLength = readInt64(MFT, pointer + 0x04, 4);
			INT64 non_residentFLag = readInt64(MFT, pointer + 0x08, 1);
			
			if (attrType == 0x10) //$STANDARD_INFORMATION case
			{
				std::cout << "Attribute Type: $STANDARD_INFORMATION" << std::endl;
				if (non_residentFLag == 0) std::cout << "Resident Attribute" << std::endl;
				else std::cout << "Non-Resident Attribute" << std::endl;

				INT64 offsetAttrData = readInt64(MFT, pointer + 0x14, 2);

				INT64 filePermissions = readInt64(MFT, pointer + offsetAttrData + 32, 4);
				std::string binaryConversion = toBinary(filePermissions);

				for (int i = binaryConversion.length() - 1; i >= 0; --i) //traversing the binary sequence to get file permissions
				{
					int size = binaryConversion.length();
					if (binaryConversion[i] == '1')
					{
						if (i == size - 1) //2^0
						{
							std::cout << "Read-only" << std::endl;
						}
						else if (i == size - 2) //2^1
						{
							std::cout << "Hidden" << std::endl;
						}
						else if (i == size - 3) //2^2
						{
							std::cout << "System" << std::endl;
						}
						else if (i == size - 6) //2^5
						{
							std::cout << "Archive" << std::endl;
						}
						else if (i == size - 7) //2^6
						{
							std::cout << "Device" << std::endl;
						}
						else { //other values that we currently don't need to consider
							std::cout << "Other type of permission" << std::endl;
						}
					}
				}
			}

			else if (attrType == 0x30) //$FILE_NAME case
			{
				std::cout << "Attribute Type: $FILE_NAME" << std::endl;
				if (non_residentFLag == 0) std::cout << "Resident Attribute" << std::endl;
				else std::cout << "Non-Resident Attribute" << std::endl;

				//in this case we look for the name of the file's name
				INT64 offsetAttrData = readInt64(MFT, pointer + 0x14, 2);
				INT64 attrFileName_Start = pointer + offsetAttrData;

				INT64 attrFileName_Length = readInt64(MFT, attrFileName_Start + 0x40, 1);
				INT64 attrFileName_NameSpace = readInt64(MFT, attrFileName_Start + 0x41, 1);

			}

			pointer += totalAttrLength;
		} while (pointer <= MFT_entry_size);
	}
};
