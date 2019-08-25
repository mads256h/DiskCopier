#include "cd_drive.h"
#include "win32_exception.h"
#include "read_error.h"

#include <iostream>

#include <fileapi.h>
#include <winioctl.h>



void cd_drive::lock_unlock_drive(const bool lock) const
{
	PREVENT_MEDIA_REMOVAL pmr = { static_cast<BOOLEAN>(lock ? TRUE : FALSE) };
	DWORD bytes = 0;
	if (!DeviceIoControl(h_drive_, IOCTL_STORAGE_MEDIA_REMOVAL, &pmr, sizeof(pmr), NULL, NULL, &bytes, NULL))
	{
		if (lock) throw win32_exception(GetLastError(), "Could not lock the drive.");

		throw win32_exception(GetLastError(), "Could not unlock the drive.");
	}
}

void cd_drive::dismount_volume() const
{
	if (DWORD bytes = 0; !DeviceIoControl(h_drive_, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes, NULL))
	{
		throw win32_exception(GetLastError(), "Could not dismount volume.");
	}
}

cd_drive::cd_drive(const std::string_view drive_path) : drive_path_(drive_path)
{
	h_drive_ = CreateFileA(drive_path_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);

	if (h_drive_ == INVALID_HANDLE_VALUE)
	{
		throw win32_exception(GetLastError(), "Could not get handle to CD drive.");
	}
}

cd_drive::~cd_drive()
{
	CloseHandle(h_drive_);
}

void cd_drive::lock_drive() const
{
	lock_unlock_drive(true);
}

void cd_drive::unlock_drive() const
{
	lock_unlock_drive(false);
}

void cd_drive::lock_volume() const
{
	if (DWORD bytes = 0; !DeviceIoControl(h_drive_, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL))
	{
		throw win32_exception(GetLastError(), "Could not lock volume.");
	}
}

void cd_drive::unlock_volume() const
{
	if (DWORD bytes = 0; !DeviceIoControl(h_drive_, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &bytes, NULL))
	{
		throw win32_exception(GetLastError(), "Could not unlock volume.");
	}
}

void cd_drive::eject() const
{
	try
	{
		dismount_volume();
	}
	catch (win32_exception& e)
	{
		std::cout << e.what();
	}

	try
	{
		unlock_drive();
	}
	catch (win32_exception& e)
	{
		std::cout << e.what();
	}


	if (DWORD bytes = 0; !DeviceIoControl(h_drive_, IOCTL_STORAGE_EJECT_MEDIA, NULL, 0, NULL, 0, &bytes, NULL))
	{
		throw win32_exception(GetLastError(), "Could not eject.");
	}
}

void cd_drive::inject() const
{
	if (DWORD bytes = 0; !DeviceIoControl(h_drive_, IOCTL_STORAGE_LOAD_MEDIA, NULL, 0, NULL, 0, &bytes, NULL))
	{
		throw win32_exception(GetLastError(), "Could not inject.");
	}
}

uint64_t cd_drive::disk_size() const
{
	DWORD bytes = 0;
	GET_LENGTH_INFORMATION gli;
	if (!DeviceIoControl(h_drive_, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(gli), &bytes, NULL))
	{
		throw win32_exception(GetLastError(), "Could not get disk size.");
	}

	return gli.Length.QuadPart;
}

constexpr uint64_t cd_drive::sector_size()
{
	return 2048;
}

uint64_t cd_drive::sector_count() const
{
	return disk_size() / sector_size();
}

std::array<char, cd_drive::sector_size()> cd_drive::read_sector(const uint64_t sector) const
{
	LARGE_INTEGER li;
	li.QuadPart = sector * sector_size();

	std::array<char, sector_size()> buffer = {};
	if (!SetFilePointerEx(h_drive_, li, NULL, FILE_BEGIN))
	{
		throw win32_exception(GetLastError(), "Could not set FilePointer.");
	}

	if (DWORD bytes = 0; !(ReadFile(h_drive_, buffer.data(), sector_size(), &bytes, NULL) && bytes == sector_size()))
	{
		const DWORD e = GetLastError();

		if (e == ERROR_CRC)
		{
			throw read_error();
		}
		if (bytes != sector_size())
		{
			throw std::exception("Invalid bytes read");
		}


		throw win32_exception(e, "Unknown error occured while reading sector");
	}

	return buffer;
}

void cd_drive::allow_extended_dasd_io() const
{
	if (DWORD bytes = 0; !DeviceIoControl(h_drive_, FSCTL_ALLOW_EXTENDED_DASD_IO, NULL, 0, NULL, 0, &bytes, NULL))
	{
		throw win32_exception(GetLastError(), "Could not allow extended DASD IO.");
	}
}

