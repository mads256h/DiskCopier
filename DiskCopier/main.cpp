// DiskCopier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "cd_drive.h"
#include "read_error.h"
#include "win32_exception.h"

#include <cstdlib>
#include <iostream>
#include <fstream>


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

	try
	{
		const cd_drive drive(argv[3]);

		drive.lock_drive();
		drive.lock_volume();
		drive.allow_extended_dasd_io();

		const uint64_t size = drive.disk_size();
		const uint64_t sectors = drive.sector_count();

		std::cout << "Disk size: " << size << '\n';
		std::cout << "Sectors: " << sectors << '\n';
		

		std::ofstream outputStream(argv[2], std::ofstream::binary);
		
		for (uint64_t i = 0; i < sectors; i++)
		{
			try
			{
				std::array<char, 2048> sector_data(drive.read_sector(i, retries));
				outputStream.write(sector_data.data(), sector_data.size());
			}
			catch (read_error)
			{
				std::cout << "Read error at sector " << i << " writing zeros!\n";
				std::array<char, 2048> empty{};
				outputStream.write(empty.data(), empty.size());
			}
		}

		drive.eject();
	}
	catch (win32_exception &e)
	{
		std::cout << e.what();
	}
	
}


