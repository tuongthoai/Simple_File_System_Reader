// NTFS_FS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "NTFS.h"

int main()
{
    // xử lí nhập tên ổ đĩa
    std::wstring disk_name = L"E";
    std::cout << "Nhap ten o dia: ";
    //std::wcin >> disk_name;
    disk_name = L"\\\\.\\" + disk_name + L":";
    LPCWSTR drive = disk_name.c_str();
    NTFS* fs = new NTFS(drive);
    fs->Print_BootSector();

    fs->printComponents();
    fs->ReadMFT();
    /*fat32.Print_Directory_File_Tree();*/
    //fat32.MENU();
    fs->interactWithUser();

    delete fs;

    //NTFS* fs = new NTFS(drive);

    //delete fs;
}


