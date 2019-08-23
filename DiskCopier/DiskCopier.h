#pragma once


#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <winioctl.h>


HANDLE GetCdHandle(char* path);

BOOL WINAPI ConsoleHandler(DWORD signal);

void SetConsoleHandler(HANDLE hCd);

void LockUnlock(bool lock, HANDLE hCd);

void Eject(HANDLE hCd);

void AllowExtendedDasdIO(HANDLE hCd);

GET_LENGTH_INFORMATION GetDiskLength(HANDLE hCd);