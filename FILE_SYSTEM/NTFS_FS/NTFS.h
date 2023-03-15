#pragma once
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <functional>
#include <stack>
#include <algorithm>
#include "Abstract_File.h"
#include "File.h"
#include "Folder.h"

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

	UINT32 fromClusterToSector(UINT32 clusterth) {
		return clusterth * this->sectors_per_cluster;
		//std::cout << "SECTOR: " << this->sectors_of_bootsector + this->numbers_of_fats * this->sector_per_FAT + (clusterth - 2) * this->sectors_per_cluster << '\n';
		//return this->sectors_of_bootsector + this->numbers_of_fats * this->sector_per_FAT + (clusterth - 2) * this->sectors_per_cluster;
	}

	void Print_BootSector()
	{
		std::cout << "\n------------------------------------------------------------------------------------------------\n";
		std::cout << "\t \t\ \t \t \t \t BOOT SECTOR : " << std::endl;
		std::cout << "Type of File System: NTFS" << std::endl;
		std::cout << "Bytes per sector: " << this->bytes_per_sector << std::endl;
		std::cout << "Sector per cluster: " << this->sectors_per_cluster << std::endl;
		std::cout << "Number of sectors in the disk: " << this->disk_sector << std::endl;
		std::cout << "First cluster of MFT: " << this->MFT_begin_cluster << std::endl;
		std::cout << "Size of MFT entry: " << this->MFT_entry_size << std::endl;
		std::cout << std::endl;
	}

	void traversePreOrder(std::string& res, Abstract_File* node, std::string _padding, std::string _ptr) {
		if (node != nullptr) {
			res += _padding;
			res += node->toString();
			res += '\n';

			std::string padding = _padding + "\t";

			if (node->getFileState() == "DIRECTORY") {
				std::vector<Abstract_File*> list = ((Folder*)node)->getContext();
				for (int i = 0; i < (int)list.size(); ++i) {
					if (i == (int)list.size() - 1) traversePreOrder(res, list[i], padding, padding);
					else {
						traversePreOrder(res, list[i], padding, padding);
					}
				}
			}
		}
	}

	void printComponents(Abstract_File* _root = nullptr) {
		std::cout << "------------------------------------------------------------------------------------------------\nROOT DIRECTORY TREE:\n";

		std::string res = "";

		if (_root) traversePreOrder(res, _root, "", "");
		else traversePreOrder(res, root, "", "");

		std::cout << res << std::endl;
	}

	std::vector<std::string> stringSpliter(std::string src) {
		std::stringstream ss(src);
		std::vector<std::string> tokens;
		std::string token;
		while (ss >> token) {
			tokens.push_back(token);
		}
		token = "";
		for (int i = 1; i < (int)tokens.size(); ++i) {
			if (token.size() == 0) token += tokens[i];
			else token += " " + tokens[i];
		}

		while (tokens.size() > 1) {
			tokens.pop_back();
		}

		tokens.push_back(token);
		return tokens;
	}

	void interactWithUser() {
		std::string usrCmd;
		std::string rootPath = "root/\t";

		Abstract_File* ptr = root;
		std::stack<Abstract_File*> st;
		std::cout << "\n";
		std::getline(std::cin, usrCmd);
		try {
			while (true) {
				printComponents(ptr);

				std::cout << rootPath << '\n';
				std::getline(std::cin, usrCmd);

				if (usrCmd == "") continue;

				std::vector<std::string> toks = stringSpliter(usrCmd);
				if (toks.size() != 2) {
					throw std::bad_function_call();
				}

				std::string cmd = toks[0];
				std::string fName = toks[1];

				if (cmd == "cd") {
					if (fName == ".") continue;
					else {
						if (fName == "..") {
							if (!st.empty()) {
								ptr = st.top();
								st.pop();
							}
							else continue;
						}
						else {
							Folder* curD = (Folder*)ptr;

							for (auto p : curD->getContext()) {
								if (p->getFileState() == "DIRECTORY" && p->getName().find(fName) != std::string::npos) {
									st.push(ptr);
									ptr = p;
									break;
								}
							}
						}
					}
				}
				else {
					if (cmd == "touch") {
						Folder* curD = (Folder*)ptr;

						for (auto p : curD->getContext()) {
							if (p->getName().find(fName) != std::string::npos) {
								if (p->getFileState() == "DIRECTORY") {
									printComponents(p);
								}
								else {
									File* f = (File*)p;

									if (f->getName().find(".txt") != std::string::npos) {
										UINT32 byteToRead = f->getFileSize();
										std::vector<UINT32> clusters = f->getClusters();
										UINT32 bytePerCluster = this->bytes_per_sector * this->sectors_per_cluster;
										PBYTE data = new BYTE[bytePerCluster + 1];

										for (int i = 0; i < (int)clusters.size(); ++i) {
											if (byteToRead == 0) break;
											int cnt = Read_Sector(data, fromClusterToSector(clusters[i]) * this->bytes_per_sector, bytePerCluster);
											if (cnt == -1) {
												std::cout << "READ FILE ERROR!";
												break;
											}
											if (byteToRead > bytePerCluster) data[bytePerCluster] = 0;
											else data[byteToRead] = 0;
											std::cout << (char*)(data);

											byteToRead -= (byteToRead < bytePerCluster ? byteToRead : bytePerCluster);
										}
									}
									else {
										std::cout << "VUI LONG DUNG UNG DUNG KHAC DE DOC\n";
									}

									std::cout << std::endl;
								}
							}
						}
					}
				}

				if (usrCmd == "quit") break;
			}
		}
		catch (std::bad_function_call& e) {
			std::cout << "ERROR: Bad function call\n";
		}
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

	void Print_Sector(BYTE* sector)
	{
		int count = 0;
		int num = 0;

		std::cout << "         0  1  2  3  4  5  6  7    8  9  A  B  C  D  E  F\n";

		std::cout << "0x0" << num << "0  ";
		bool flag = 0;
		for (int i = 0; i < 512; i++)
		{
			count++;
			if (i % 8 == 0)
				std::cout << "  ";
			printf("%02X ", sector[i]);
			if (i == 255)
			{
				flag = 1;
				num = 0;
			}

			if (i == 511) break;
			if (count == 16)
			{
				int index = i;

				std::cout << std::endl;

				if (flag == 0)
				{
					num++;
					if (num < 10)
						std::cout << "0x0" << num << "0  ";
					else
					{
						char hex = char(num - 10 + 'A');
						std::cout << "0x0" << hex << "0  ";
					}

				}
				else
				{
					if (num < 10)
						std::cout << "0x1" << num << "0  ";
					else
					{
						char hex = char(num - 10 + 'A');
						std::cout << "0x1" << hex << "0  ";
					}
					num++;
				}

				count = 0;
			}
		}
		std::cout << std::endl;
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
		INT64 firstSectorOfMFT = this->MFT_begin_cluster * this->sectors_per_cluster * this->bytes_per_sector;
		PBYTE buffer = new BYTE(this->MFT_entry_size);
		this->Read_Sector(buffer, firstSectorOfMFT, this->MFT_entry_size);
		
		this->Print_Sector(buffer);

		;;;;;		std::string Signature = Get_String(MFT, 0x00, 4);
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
