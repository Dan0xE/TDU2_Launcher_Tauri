#include <windows.h>
#include <stdbool.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <stdint.h>
#include <process.h>
#include <direct.h>
#include <io.h>
#include <stdlib.h>

#define PROCESS_NAME "TestDrive2.exe"
#define BUFFER_SIZE 1024
#define MAX_PATH 260
#define EXE_NAME "TestDrive2.exe"
#define MUTEX_NAME "957e4cc3"

bool DoesFileExist(const char *name)
{
	FILE *file = fopen(name, "r");
	if (file)
	{
		fclose(file);
		return true;
	}
	return false;
}

DWORD GetProcessIdFunc()
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(pe32);
	if (Process32First(hSnap, &pe32))
	{
		do
		{
			if (strcmp(pe32.szExeFile, PROCESS_NAME) == 0)
			{
				CloseHandle(hSnap);
				return pe32.th32ProcessID;
			}
		} while (Process32Next(hSnap, &pe32));
	}
	CloseHandle(hSnap);
	return 0;
}

uintptr_t GetModuleBaseAddress(DWORD pid, const char *modName)
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (strcmp(modEntry.szModule, modName) == 0)
				{
					CloseHandle(hSnap);
					return (uintptr_t)modEntry.modBaseAddr;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
		CloseHandle(hSnap);
	}
	return 0;
}

bool LoadLibraryInject(DWORD ProcessId, const char *Dll)
{
	char CustomDLL[MAX_PATH];
	GetFullPathName(Dll, MAX_PATH, CustomDLL, 0);
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessId);
	LPVOID allocatedMem = VirtualAllocEx(hProcess, NULL, sizeof(CustomDLL), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!WriteProcessMemory(hProcess, allocatedMem, CustomDLL, sizeof(CustomDLL), NULL))
	{
		CloseHandle(hProcess);
		return false;
	}

	CreateRemoteThread(hProcess, 0, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, allocatedMem, 0, 0);

	CloseHandle(hProcess);
	return true;
}

LPVOID getNTOpenFile()
{
	return GetProcAddress(LoadLibraryW(L"ntdll"), "NtOpenFile");
}

LPVOID NTOpenFile = NULL;

bool ExecuteBypass(HANDLE hProcess)
{
	if (!NTOpenFile)
	{
		NTOpenFile = getNTOpenFile();
	}
	if (NTOpenFile)
	{
		char originalBytes[5];
		memcpy(originalBytes, NTOpenFile, 5);
		if (WriteProcessMemory(hProcess, NTOpenFile, originalBytes, 5, NULL))
		{
			return true;
		}
	}
	return false;
}

bool Backup(HANDLE hProcess)
{
	if (!NTOpenFile)
	{
		NTOpenFile = getNTOpenFile();
	}
	if (NTOpenFile)
	{
		char Orig[5];
		memcpy(Orig, NTOpenFile, 5);
		WriteProcessMemory(hProcess, NTOpenFile, Orig, 0, NULL);
		return true;
	}
	return false;
}

__declspec(dllexport) bool InjectDLL(const char *DllPath)
{
	if (!DoesFileExist(DllPath))
	{
		return false;
	}
	return LoadLibraryInject(GetProcessIdFunc(), DllPath);
}

#define BUFFER_SIZE 1024
#define MAX_PATH 260
#define EXE_NAME "TestDrive2.exe"
#define MUTEX_NAME "957e4cc3"

void show_message(const char *title, const char *message)
{
	MessageBox(NULL, message, title, MB_ICONEXCLAMATION | MB_OK);
}

HANDLE create_mutex(const char *name)
{
	HANDLE mutex = CreateMutex(NULL, FALSE, name);

	if (mutex == NULL)
	{
		fprintf(stderr, "Failed to create mutex: %d\n", GetLastError());
		exit(EXIT_FAILURE);
	}
	return mutex;
}

__declspec(dllexport) bool is_process_running()
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);
	BOOL process_found = FALSE;

	if (Process32First(snapshot, &entry))
	{
		do
		{
			if (_stricmp(entry.szExeFile, EXE_NAME) == 0)
			{
				process_found = TRUE;
				break;
			}

		} while (Process32Next(snapshot, &entry));
	}
	CloseHandle(snapshot);
	return process_found;
}

void run_game(const char *path)
{
	char exe_path[BUFFER_SIZE];
	char steam_api_path[BUFFER_SIZE];
	char ul_steam_api_path[BUFFER_SIZE];
	char exe_path_quoted[BUFFER_SIZE];

	snprintf(exe_path, BUFFER_SIZE, "%s/TestDrive2.exe", path);
	snprintf(steam_api_path, BUFFER_SIZE, "%s/steam_api.dll", path);
	snprintf(ul_steam_api_path, BUFFER_SIZE, "%s/UL_steam_api.dll", path);
	snprintf(exe_path_quoted, BUFFER_SIZE, "\"%s\"", exe_path);

	if (_access(exe_path, 0) != -1)
	{
		if (_access(steam_api_path, 0) != -1 && rename(steam_api_path, ul_steam_api_path) != 0)
		{
			fprintf(stderr, "Failed to rename %s to %s\n", steam_api_path, ul_steam_api_path);
			exit(EXIT_FAILURE);
		}

		if (_chdir(path) != 0)
		{
			printf("path: %s", path);
			perror("Failed to change dir");
			exit(EXIT_FAILURE);
		}

		system(exe_path_quoted);

		if (_access(ul_steam_api_path, 0) != -1 && rename(ul_steam_api_path, steam_api_path) != 0)
		{
			fprintf(stderr, "Failed to rename %s to %s\n", ul_steam_api_path, steam_api_path);
			exit(EXIT_FAILURE);
		}
	}
	else
	{
		fprintf(stderr, "The provided path %s does not exist!\n", path);
	}
}

__declspec(dllexport) void run_game_with_mutex(const char *path)
{
	if (is_process_running(EXE_NAME))
	{
		fprintf(stderr, "Game is already running!\n");
		return;
	}

	HANDLE mutex = create_mutex(MUTEX_NAME);
	run_game(path);

	if (WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0)
	{
		fprintf(stderr, "Failed to wait for mutex: %d\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	if (!ReleaseMutex(mutex))
	{
		fprintf(stderr, "Failed to relase the mutex: %d\n", GetLastError());
		exit(EXIT_FAILURE);
	}

	if (!CloseHandle(mutex))
	{
		fprintf(stderr, "Failed to close mutex handle: %d\n", GetLastError());
		exit(EXIT_FAILURE);
	}
	exit(0);
}