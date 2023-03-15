// FILE_SYSTEM.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "FAT_32.h"

int main()
{
    // xử lí nhập tên ổ đĩa
    std::wstring disk_name = L"F";
    std::cout << "Nhap ten o dia: ";
    std::wcin >> disk_name;
    disk_name = L"\\\\.\\" + disk_name + L":";
    LPCWSTR drive = disk_name.c_str();
    FAT_32* fs = new FAT_32(drive);
    fs->Print_BootSector();

    fs->printComponents();
    /*fat32.Print_Directory_File_Tree();*/
    //fat32.MENU();
    fs->interactWithUser();

    delete fs;

    //NTFS* fs = new NTFS(drive);

    //delete fs;
}
