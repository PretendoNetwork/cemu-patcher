#include <windows.h>
#include <fstream>
#include <string>
#include <iostream>

void HookCurl()
{
    DWORD current_process_id = GetCurrentProcessId();
    HANDLE cemu_process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, current_process_id);

    if (cemu_process)
    {
        //MessageBox(NULL, "CemuHook found. Attempting injection. This is widely untested and not officially support by Rajkosto. Use at your own risk", "Warning", MB_ICONWARNING);

        char cemuhook_path[MAX_PATH];
        GetFullPathName("CurlHook.dll", MAX_PATH, cemuhook_path, NULL);

        long cemuhook_path_size = strlen(cemuhook_path) + 1;

        LPVOID cemuhook_memory = VirtualAllocEx(cemu_process, NULL, cemuhook_path_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (cemuhook_memory == NULL)
        {
            MessageBox(NULL, "Failed to allocate memory in Cemu", "Error", MB_ICONERROR);
            return;
        }

        if (!WriteProcessMemory(cemu_process, cemuhook_memory, cemuhook_path, cemuhook_path_size, 0))
        {
            MessageBox(NULL, "Failed to write to Cemu process", "Error", MB_ICONERROR);
            return;
        }

        LPTHREAD_START_ROUTINE load_library_a_addr = (LPTHREAD_START_ROUTINE)GetProcAddress(LoadLibrary("kernel32"), "LoadLibraryA");
        if (!CreateRemoteThread(cemu_process, NULL, 0, load_library_a_addr, cemuhook_memory, 0, NULL))
        {
            MessageBox(NULL, "Failed to create Remote Thread", "Error", MB_ICONERROR);
            return;
        }

        MessageBox(NULL, "Injected CemuHook", "Success", MB_OK);
    }
}