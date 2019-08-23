// DiskCopier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "DiskCopier.h"

#include <cstdlib>
#include <iostream>
#include <fstream>

#include <fileapi.h>


// The usable sector size in bytes on CDs and DVDs
constexpr LONGLONG CDROM_SECTOR_SIZE = 2048;

volatile bool stop = false;

HANDLE _hCd = nullptr;
BOOL WINAPI ConsoleHandler(DWORD signal)
{
	if (signal == CTRL_C_EVENT)
	{
		LockUnlock(false, _hCd);
	}
	
	return true;
}

HANDLE GetCdHandle(char* path)
{
	DWORD bytes = 0;
	SetLastError(0);
	HANDLE ret = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (!ret)
	{
		std::cout << "Could not get handle to CD drive. Error code: " << GetLastError() << ".\n";
		exit(1);
	}
	return ret;
}

void SetConsoleHandler(HANDLE hCd)
{
	_hCd = hCd;
	if (!SetConsoleCtrlHandler(ConsoleHandler, TRUE)) {
		std::cout << "ERROR: Could not set control handler. Error: " << GetLastError() << ".\n";
		exit(1);
	}
}

void LockUnlock(bool lock, HANDLE hCd)
{
	SetLastError(0);
	PREVENT_MEDIA_REMOVAL pmr = {static_cast<BOOLEAN>(lock ? TRUE : FALSE)};
	DWORD bytes = 0;
	if (!DeviceIoControl(hCd, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(pmr), NULL, NULL, &bytes, NULL))
	{
		std::cout << "Could not " << (lock ? "lock_drive" : "unlock") << " the drive. Error: " << GetLastError() << ".\n";
	}
}

void Eject(HANDLE hCd)
{
	DWORD bytes = 0;
	SetLastError(0);
	if (!DeviceIoControl(hCd, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL))
	{
		std::cout << "Could not lock_drive volume. Error: " << GetLastError() << ".\n";
	}

	SetLastError(0);
	if (!DeviceIoControl(hCd, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes, NULL))
	{
		std::cout << "Could not dismount volume. Error: " << GetLastError() << ".\n";
	}

	LockUnlock(false, hCd);
	
	SetLastError(0);
	if (!DeviceIoControl(hCd, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &bytes, NULL))
	{
		std::cout << "Could not eject the drive. Error: " << GetLastError() << ".\n";
	}
}

void AllowExtendedDasdIO(HANDLE hCd)
{
	DWORD bytes = 0;
	if (!DeviceIoControl(hCd, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &bytes, NULL))
	{
		std::cout << "Could not allow extended dasd io. Error: " << GetLastError() << ".\n";
		exit(1);
	}
}

GET_LENGTH_INFORMATION GetDiskLength(HANDLE hCd)
{
	DWORD bytes = 0;
	GET_LENGTH_INFORMATION gli;
	if (!DeviceIoControl(hCd, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli), &bytes, NULL))
	{
		std::cout << "Could not get disk length info. Error: " << GetLastError() << ".\n";
		exit(1);
	}

	return gli;
}

int main(int argc, char* argv[])
{
	if (argc != 4)
	{
		std::cout << "DiskCopier.exe retries outputfile diskdrive\n";
		return 1;
	}
	
	const long retries = strtol(argv[1], nullptr, 10);

	if (retries == LONG_MIN || retries == LONG_MAX)
	{
		std::cout << "retries argument is not a valid number.\n";
		return 1;
	}


	
	HANDLE hCd = GetCdHandle(argv[3]);
	

	SetConsoleHandler(hCd);

	
	LockUnlock(true, hCd);

	AllowExtendedDasdIO(hCd);

	const GET_LENGTH_INFORMATION gli = GetDiskLength(hCd);

	const LONGLONG totalSectors = gli.Length.QuadPart / CDROM_SECTOR_SIZE;
	
	std::cout << "Disk length: " << gli.Length.QuadPart << " bytes.\n";
	std::cout << "Total sectors: " << totalSectors << ".\n";
	

	char buf[CDROM_SECTOR_SIZE] = { 0 };
	constexpr char empty[CDROM_SECTOR_SIZE] = { 0 };


	std::ofstream outputFile(argv[2], std::ofstream::binary);

	LONGLONG successSectors = 0;
	LONGLONG recoveredSectors = 0;
	LONGLONG failedSectors = 0;

	for (LONGLONG i = 0; i < totalSectors; i++)
	{
		LARGE_INTEGER li;
		li.QuadPart = i * CDROM_SECTOR_SIZE;
		for (int tries = 0; tries < retries; tries++)
		{
			if (!SetFilePointerEx(hCd, li, NULL, FILE_BEGIN))
			{
				std::cout << "SetFilePointer failed: " << GetLastError() << ". Exiting...\n";
				LockUnlock(false, hCd);
				return 1;
			}

			if (DWORD bytes = 0; !(ReadFile(hCd, buf, CDROM_SECTOR_SIZE, &bytes, NULL) && bytes == CDROM_SECTOR_SIZE))
			{
				const DWORD e = GetLastError();

				if (e == ERROR_CRC)
				{
					if (tries == 0) recoveredSectors++;
					std::cout << "Read error in sector " << i << "Try " << tries << " out of " << retries << ".\n";
					continue;
				}
				
				std::cout << "An unexpected error occured: " << e << ". Exiting...\n";
				LockUnlock(false, hCd);
				return 1;
			}

			if (tries == 0) successSectors++;
			try
			{
				outputFile.write(buf, CDROM_SECTOR_SIZE);
			}
			catch (std::ofstream::failure &e_failure)
			{
				std::cout << "Could not write to file: " << e_failure.what() << '\n';
				return 1;
			}
			
			goto outerend;
		}
		recoveredSectors--;
		failedSectors++;

		std::cout << "Sector " << i << " failed! Writing zeros!\n";
		
		try
		{
			outputFile.write(empty, CDROM_SECTOR_SIZE);
		}
		catch (std::ofstream::failure &e_failure)
		{
			std::cout << "Could not write to file: " << e_failure.what() << '\n';
			return 1;
		}
		
	outerend:;
	}
	
	/*for (LONGLONG i = 0; i < totalSectors; i++)
	{	
		int tries = 0;
		tryagain:
		if (stop == true)
		{
			std::cout << "Stopping program\n";
			LockUnlock(false, hCd);
			Eject(hCd);
			
			return 0;
		}
		tries++;
		SetLastError(0);
		LARGE_INTEGER li;
		li.QuadPart = i * CDROM_SECTOR_SIZE;
		if (!SetFilePointerEx(hCd, li, NULL, FILE_BEGIN))
		{
			std::cout << "SetFilePointer failed: " << GetLastError() << ".\n";
		}
		SetLastError(0);
		if (DWORD bytes = 0; !(ReadFile(hCd, buf, CDROM_SECTOR_SIZE, &bytes, NULL) && bytes == CDROM_SECTOR_SIZE))
		{
			const auto e = GetLastError();
			SetLastError(0);
			
			if (e == 23 && tries < retries)
			{
				if (tries == 1) recoveredSectors++;
				std::cout << "Read error in sector " << i << ". Try " << tries << " out of " << retries << ".\n";
				goto tryagain;
			}
			if (e == 23)
			{
				recoveredSectors--;
				failedSectors++;
				std::cout << "Sector " << i << " failed! Writing zeros!\n";
				outputFile.write(empty, CDROM_SECTOR_SIZE);
				continue;
			}
			if (e != 0)
			{
				std::cout << "An unexpected error occured: " << e << ". Exiting!\n";
				LockUnlock(false, hCd);
				exit(1);
			}

			if (bytes != CDROM_SECTOR_SIZE)
			{
				std::cout << "We did not read 2048 bytes from sector " << i << ". Exiting...\n";
				LockUnlock(false, hCd);
				exit(1);
			}
			
		}
		if (tries == 1) successSectors++;
		outputFile.write(buf, CDROM_SECTOR_SIZE);
	}*/

	std::cout << "Total sectors: " << totalSectors << "\n";
	std::cout << "Successful sectors: " << successSectors << "\n";
	std::cout << "Recovered sectors: " << recoveredSectors << "\n";
	std::cout << "Failed sectors: " << failedSectors << "\n";

	LockUnlock(false, hCd);
	Eject(hCd);
}


