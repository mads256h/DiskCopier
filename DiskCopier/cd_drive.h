#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <array>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define SECTOR_SIZE 2048

/**
 * \brief A class that does certain operations on a CD or DVD drive.
 */
class cd_drive final
{
	std::string drive_path_;
	HANDLE h_drive_;


	/**
	 * \brief Internal memberfunction used to lock and unlock the drive.
	 * \param lock if true locks the drive else unlocks the drive.
	 */
	void lock_unlock_drive(bool lock) const;

	/**
	 * \brief Dismounts the volume.
	 * It is used before disk ejection.
	 */
	void dismount_volume() const;



public:
	/**
	 * \brief Constructs a cd_drive object.
	 * \param drive_path Device path to the drive.
	 */
	explicit cd_drive(const std::string_view drive_path);
	/**
	 * \brief Destructs the cd_drive object.
	 * It cleans up the underlying file handle.
	 */
	~cd_drive();

	/**
	 * \brief Locks the drive so it can't eject.
	 */
	void lock_drive() const;
	/**
	 * \brief Unlocks the drive so it can eject.
	 */
	void unlock_drive() const;

	/**
	 * \brief Locks the volume to get exclusive access.
	 */
	void lock_volume() const;
	/**
	 * \brief Unlocks the volume to get shared access.
	 */
	void unlock_volume() const;


	/**
	 * \brief Ejects the disk.
	 */
	void eject() const;
	/**
	 * \brief Injects the disk.
	 */
	void inject() const;

	/**
	 * \brief Gets the size of the disk.
	 * \return The size of the disk.
	 */
	[[nodiscard]] uint64_t disk_size() const;

	/**
	 * \brief Gets the amount of sectors on disk.
	 * \return The amount of sectors on disk.
	 */
	[[nodiscard]] uint64_t sector_count() const;

	/**
	 * \brief Reads a sector from disk.
	 * \param sector Sector to read.
	 * \return Array representation of sector.
	 */
	[[nodiscard]] std::array<char, SECTOR_SIZE> read_sector(const uint64_t sector) const;


	/**
	 * \brief Allow unbounded access to the disk.
	 */
	void allow_extended_dasd_io() const;
};

