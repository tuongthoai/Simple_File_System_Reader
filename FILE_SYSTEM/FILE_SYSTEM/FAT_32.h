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

#define MBR_SIZE 512

class FAT_32 {
private:
	LPCWSTR drive;

	//BPB Info
	UINT32 bytes_per_sector;
	UINT32 sectors_per_cluster;
	UINT32 sectors_of_bootsector;
	UINT32 numbers_of_fats;
	UINT32 fatSize;
	UINT32 volumeSize;
	UINT32 first_sector_of_RDET;
	UINT32 sector_per_FAT;
	UINT32 first_sector_of_data;

	// Bytes of Data
	PBYTE FAT;
	PBYTE BootSector;
	Abstract_File* root = nullptr;
public:
	FAT_32(LPCWSTR _drive) {
		this->drive = _drive;
		this->BootSector = new BYTE[MBR_SIZE];
		this->Read_Sector(this->BootSector, 0, MBR_SIZE);
		this->bytes_per_sector = readInt(this->BootSector, 0x0B, 2);
		this->sectors_per_cluster = readInt(this->BootSector, 0x0D, 1);
		this->sectors_of_bootsector = readInt(this->BootSector, 0x0E, 2);
		this->numbers_of_fats = readInt(this->BootSector, 0x10, 1);
		this->sector_per_FAT = readInt(this->BootSector, 0x24, 4);
		this->first_sector_of_data = this->sectors_of_bootsector + this->numbers_of_fats * this->sector_per_FAT;

		this->FAT = new BYTE[sector_per_FAT * bytes_per_sector];
		this->Read_Sector(this->FAT, this->sectors_of_bootsector * this->bytes_per_sector, sector_per_FAT * bytes_per_sector);

		root = new Folder();
		root->setName("root");
		root->setFileState(16);
		root->setStartCluster(fromClusterToSector(this->first_sector_of_data));

		this->Read_RDET(this->first_sector_of_data, (Folder*)root);
	}

	~FAT_32() {
		delete FAT;
		delete BootSector;
	}

	UINT32 readInt(BYTE* sector, int offset, int number)
	{
		UINT32 k = 0;
		memcpy(&k, sector + offset, number);
		return k;
	}

	/*
	Recieve the ith sector and convert its to kth cluster
	INPUT:  UINT32
	OUTPUT: UINT32
	*/
	UINT32 fromSectorToCluster(UINT32 sectorth) {
		//std::cout << "CLUSTER: " << (sectorth - this->sectors_of_bootsector - this->numbers_of_fats * this->fatSize) / this->sectors_per_cluster + 2 << std::endl;
		return (sectorth - this->sectors_of_bootsector - this->numbers_of_fats * this->fatSize) / this->sectors_per_cluster + 2;
	}

	/*
	Recieve the kth cluster and convert its to ith sector
	INPUT:  UINT32
	OUTPUT: UINT32
	*/
	UINT32 fromClusterToSector(UINT32 clusterth) {
		//std::cout << "SECTOR: " << this->sectors_of_bootsector + this->numbers_of_fats * this->sector_per_FAT + (clusterth - 2) * this->sectors_per_cluster << '\n';
		return this->sectors_of_bootsector + this->numbers_of_fats * this->sector_per_FAT + (clusterth - 2) * this->sectors_per_cluster;
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

	/*
	* formated 512 bytes from input to a table with 16 columns and 32 rows
	*/
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


	/*
	* printing info of Boot Sector
	*/
	void Print_BootSector()
	{
		std::cout << "\n------------------------------------------------------------------------------------------------\n";
		std::cout << "\t \t\ \t \t \t \t BOOT SECTOR : " << std::endl;
		std::cout << "Type of File System: FAT32" << std::endl;
		std::cout << "Bytes per sector: " << this->bytes_per_sector << std::endl;
		std::cout << "Sector per cluster: " << this->sectors_per_cluster << std::endl;
		std::cout << "Sector of BootSector: " << this->sectors_of_bootsector << std::endl;
		std::cout << "Number of FAT table: " << this->numbers_of_fats << std::endl;
		std::cout << "Sector per FAT table: " << this->sector_per_FAT << std::endl;
		std::cout << "First sector of FAT table: " << this->sectors_of_bootsector << std::endl;
		std::cout << "First sector of RDET: " << this->sectors_of_bootsector + numbers_of_fats * sector_per_FAT << std::endl;
		std::cout << "First sector of Data: " << this->sectors_of_bootsector + numbers_of_fats * sector_per_FAT << std::endl;
		std::cout << std::endl;
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

	void Read_RDET(int sector_index, Folder* parDir)
	{
		BYTE* RDET = new BYTE[512];
		this->Read_Sector(RDET, sector_index * 512, 512);

		std::string file_name = "";
		std::wstring uniFileName = L"";
		int pointer = 0;

		int skipCount = 0;

		for (int i = 0; i < 2; ++i) {
			BYTE val = readInt(RDET, pointer + 0x0B, 1);
			if (val == 0x0F)
				while (readInt(RDET, pointer + 0x0B, 1) == 0x0F) pointer += 32;
			pointer += 32;
		}

		do {
			if (pointer == 512)
			{
				pointer = 0;
				sector_index++;
				this->Read_Sector(RDET, sector_index * 512, 512);
			}

			if (readInt(RDET, pointer + 0x0B, 1) == 0x00)
				break;

			if (readInt(RDET, pointer, 1) != 0xE5 && (readInt(RDET, pointer + 0x0B, 1) == 0x0F || readInt(RDET, pointer + 0x0B, 1) == 0x10 || readInt(RDET, pointer + 0x0B, 1) == 0x20)) {

				// if subEntry
				if (readInt(RDET, pointer + 0x0B, 1) == 0x0F)
					file_name = Get_String(RDET, pointer + 1, 10) + Get_String(RDET, pointer + 0x0E, 12) + Get_String(RDET, pointer + 0x1C, 4) + file_name;
				// if mainEntry
				else if (readInt(RDET, pointer + 0x0B, 1) == 0x10 || readInt(RDET, pointer + 0x0B, 1) == 0x20)
				{
					//cout << "entry" << endl;
					BYTE startCluster[4];

					memcpy(startCluster, RDET + (pointer + 0x14), 2);
					memcpy(startCluster + 2, RDET + (pointer + 0x1A), 2);

					std::swap(startCluster[0], startCluster[1]);
					std::swap(startCluster[2], startCluster[3]);

					UINT32 stCluster = startCluster[0] * 4096 + startCluster[1] * 256 + startCluster[2] * 16 + startCluster[3];

					UINT32 first_cluster = stCluster;
					unsigned int last_cluster = stCluster; // cluster ket thuc

					std::vector<UINT32> clusters;
					clusters.push_back(stCluster);
					while (true) {
						//cout << Get_Value_Little_Endian(FAT, last_cluster * 4, 4) << endl;
						if (readInt(FAT, last_cluster * 4, 4) == 0x0FFFFFFF || readInt(FAT, last_cluster * 4, 4) == 0x0FFFFFF8 || last_cluster == 0 || last_cluster >= 128 || first_cluster == 128)
							break;
						else  if (readInt(FAT, last_cluster * 4, 4) == 0x0FFFFFF7 || readInt(FAT, last_cluster * 4, 4) == 0)
						{
							std::cout << "Can read FAT table !!!" << std::endl;
							break;
						}
						else
						{
							last_cluster = readInt(FAT, last_cluster * 4, 4);
							clusters.push_back(last_cluster);
						}
					}

					Abstract_File* p_file = nullptr;
					if (readInt(RDET, pointer + 0x0B, 1) == 0x10) //type Folder
					{
						p_file = new Folder();
						if (file_name == "")
							file_name = Get_String(RDET, pointer, 8);

						validName(file_name);
						p_file->setName(file_name);
						p_file->setFileState(16);

						if (parDir) parDir->add(p_file);

						if (file_name != "." && file_name != "..") for (UINT32 clus : clusters) p_file->addCluster(clus);

						if (readInt(RDET, pointer + 28, 4) == 0 && file_name != "." && file_name != ".." && sector_index != fromClusterToSector(stCluster))
							this->Read_RDET(fromClusterToSector(stCluster), (Folder*)p_file);

						file_name = "";

					}
					else  if (readInt(RDET, pointer + 0x0B, 1) == 0x20) //type File
					{
						if (file_name == "")
							file_name = Get_String(RDET, pointer, 8) + "." + Get_String(RDET, pointer + 8, 3);


						p_file = new File();
						validName(file_name);
						p_file->setName(file_name);
						p_file->setFileSize(readInt(RDET, pointer + 0x1C, 4));
						p_file->setFileState(32);

						for (auto clus : clusters) p_file->addCluster(clus);
						if (parDir) parDir->add(p_file);

						file_name = "";
					}
				}
				else
					file_name = "";
			}

			pointer += 32;
		} while (true);
	}

	void Print_RDET()
	{
		std::cout << "\n------------------------------------------------------------------------------------------------\n";
		std::cout << "\t \t\ \t \t \t \t RDET : " << std::endl;
		this->Read_RDET(this->first_sector_of_data, (Folder*)root);
	}

	std::string get_utf8(const std::wstring& wstr)
	{
		if (wstr.empty()) return std::string();
		int sz = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, 0, 0, 0, 0);
		std::string res(sz, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], -1, &res[0], sz, 0, 0);
		return res;
	}

	std::wstring get_utf16(const std::string& str)
	{
		if (str.empty()) return std::wstring();
		int sz = MultiByteToWideChar(CP_UTF8, 0, &str[0], -1, 0, 0);
		std::wstring res(sz, 0);
		MultiByteToWideChar(CP_UTF8, 0, &str[0], -1, &res[0], sz);
		return res;
	}

	void validName(std::string& name) {
		while (name.back() == ' ') name.pop_back();
		while (name.back() == -1) name.pop_back();
	}

	/*Rrinting components of a Disk*/
	void printComponents(Abstract_File* _root = nullptr) {
		std::cout << "------------------------------------------------------------------------------------------------\nROOT DIRECTORY TREE:\n";

		std::string res = "";

		if (_root) traversePreOrder(res, _root, "", "");
		else traversePreOrder(res, root, "", "");

		std::cout << res << std::endl;
	}

	/*
	* Traverse through all components of a disk
	*/
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


	/*
	Split a string by space character using stringstream
	INPUT: a string
	OUTPUT: vector of strings
	*/
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

	/*
	Checking if a dir is contain a abstract file name start with dirName
	INPUT: 
		curDir: Pointer of Abstract_File
		dirName: name of the file
	OUPUT: 
		a pointer to the file start with dirName
		nullptr if cannot find any file start with dirName
	*/
	Abstract_File* findNextDir(Abstract_File* curDir, std::string dirName) {
		Folder* curD = (Folder*)curDir;
		for (auto p : curD->getContext()) {
			if (p->getFileState() == "DIRECTORY" && (p->getName().find(dirName) != std::string::npos)) return p;
		}

		return nullptr;
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

				std::cout << rootPath <<'\n';
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
};