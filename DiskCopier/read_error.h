#pragma once
#include <exception>

/**
 * \brief Used to indicate that a sector failed to read.
 */
class read_error final : std::exception
{
};

