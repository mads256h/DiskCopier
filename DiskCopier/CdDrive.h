#pragma once

#include "win32exception.h"

#include <string>
#include <string_view>
#include <cstdint>
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class cd_drive
{
	std::string drive_path_;
	HANDLE h_drive_;

	
	void lock_unlock_drive(bool lock) const;

	void dismount_volume() const;

	

public:
	explicit cd_drive(const std::string_view drive_path);
	~cd_drive();

	void lock_drive() const;
	void unlock_drive() const;

	void lock_volume() const;
	void unlock_volume() const;
	

	void eject() const;
	void inject() const;

	[[nodiscard]] uint64_t disk_size() const;
	[[nodiscard]] static uint64_t sector_size();
	[[nodiscard]] uint64_t sector_count() const;

	[[nodiscard]] std::array<char, 2048> read_sector(const uint64_t sector, const int retries) const;

	//Allows full access to the drive content.
	void allow_extended_dasd_io() const;
};

