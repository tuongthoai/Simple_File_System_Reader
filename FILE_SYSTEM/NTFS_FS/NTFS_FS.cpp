// NTFS_FS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "NTFS.h"

int main()
{
	char driveName;
	std::cout << "Enter drive name: "; 
	std::cin >> driveName;

	NTFS fs(driveName);
	fs.readInfo();

	while (true) {
		system("cls");
		std::cout << " Volume " << (char)toupper(driveName) << " - NTFS\n\n";
		std::cout << " 1. XEM NOI DUNG\n";
		std::cout << " 2. IN CAY THU MUC\n";
		std::cout << " 0. Exit\n\n";
		int choice;
		std::cout << " LUA CHON: "; std::cin >> choice;
		switch (choice)
		{
		case 0:
			return 0;
		case 1:
			fs.printInfo();
			system("pause");
			break;
		case 2:
			fs.displayDirectory();
			break;
		default:
			break;
		}
	}
}


