// dllmain.cpp: This is where the magic happens!

/**
 AriseFix (C) 2021 Bryce Q.

 AriseFix is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 AriseFix is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.
**/

#define _CRT_SECURE_NO_WARNINGS
#define _USE_MATH_DEFINES

#include "../Source/pch.h"
#include "wtypes.h"
#include <Windows.h>
#include <process.h>
#include "Plugins/Arise.h"

AriseFix AF = AriseFix(); // Declares a non-static version of the AriseFix class for hooking.

void StartPatch()
{
    Sleep(5000); // Sleeps the thread for five seconds before starting the patching process.
    AF.StartPatch();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ReasonForCall, LPVOID lpReserved)
{
    switch (ReasonForCall)
    {
    case DLL_PROCESS_ATTACH:
		HANDLE patchThread; // Creates a handle to the patch thread, so it can be closed easier
		patchThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall*)(void*))StartPatch, NULL, 0, 0); // Calls the StartPatch function in a new thread on start.
		// CloseHandle(patchThread); // Closes the StartPatch thread handle.
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}