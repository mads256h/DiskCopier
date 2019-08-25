// DiskCopier.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "cd_drive.h"
#include "read_error.h"
#include "win32_exception.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>



volatile bool stop = false;

/**
 * \brief Used to handle console closing operations.
 * \return TRUE
 */
BOOL WINAPI ConsoleCtrlHandler(DWORD)
{
	stop = true;

	return TRUE;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "DiskCopier.exe outputfile diskdrive\n";
		return EXIT_FAILURE;
	}



	if (!SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE))
	{
		std::cout << "Could not set console ctrl handler: " << GetLastError() << '\n';
		return EXIT_FAILURE;
	}

	try
	{
		const cd_drive drive(argv[2]);

		drive.lock_drive();
		drive.lock_volume();
		drive.allow_extended_dasd_io();

		const uint64_t size = drive.disk_size();
		const uint64_t sectors = drive.sector_count();

		std::cout << "Disk size: " << size << '\n';
		std::cout << "Sectors: " << sectors << '\n';

		std::vector<uint64_t> failed_sectors;

		std::ofstream output_stream(argv[1], std::ofstream::binary);

		for (uint64_t i = 0; i < sectors; i++)
		{
			if (stop)
			{
				goto end;
			}

			try
			{
				const std::array<char, SECTOR_SIZE> sector_data(drive.read_sector(i));
				output_stream.write(sector_data.data(), sector_data.size());
			}
			catch (read_error&)
			{
				std::cout << "Read error at sector " << i << " writing zeros!\n";
				constexpr std::array<char, SECTOR_SIZE> empty{};
				output_stream.write(empty.data(), empty.size());
				failed_sectors.push_back(i);
			}
		}

		std::cout << "Trying to recover bad sectors.\n";

		while (!failed_sectors.empty())
		{
			for (size_t i = 0; i < failed_sectors.size(); i++)
			{
				if (stop)
				{
					goto end;
				}

				try
				{
					const std::array<char, SECTOR_SIZE> sector_data(drive.read_sector(failed_sectors[i]));
					output_stream.seekp(failed_sectors[i] * SECTOR_SIZE);
					output_stream.write(sector_data.data(), sector_data.size());
					std::cout << "Recovered sector " << failed_sectors[i] << '\n';
					failed_sectors.erase(failed_sectors.begin() + i);
				}
				catch (read_error&)
				{
					std::cout << "Read failed at sector " << failed_sectors[i] << '\n';
				}
			}
		}
	end:

		drive.eject();
	}
	catch (win32_exception& e)
	{
		std::cout << e.what();
	}

	return EXIT_SUCCESS;
}
