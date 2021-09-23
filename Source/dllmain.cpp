// dllmain.cpp: This is where the magic happens!

/**
 EDFix (C) 2020 Bryce Q.

 EDFix is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 EDFix is distributed in the hope that it will be useful,
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
#include <iostream>
#include <fstream>
#include <math.h>
#include <process.h>
#include <stdio.h>
#include "../Source/ThirdParty/inireader/IniReader.h"
#include "../Source/ThirdParty/ModUtils/MemoryMgr.h"
#include "config.h"
#include "offsets.h"

using namespace std;

// Resolution and Aspect Ratio variables
int hRes;
int vRes;
float aspectRatio;
bool preserveYAxis;

// FOV variables
float originalFOV = 0.008726646192; // Declares the original 16:9 vertical FOV.
float combatFOV;
float explorationFOV;
float FOVOffset;

// Misc variables
bool check = true; // do not change to false or else resolution checks won't run.

// Process HMODULE variable
HMODULE baseModule = GetModuleHandle(NULL);

void readConfig()
{
    cout.flush();
    //freopen(FILE**)stdout, "CONOUT$", "w", stdout); // Allows us to add outputs to the ASI Loader Console Window.
    cout.clear();
    cin.clear();
    CIniReader config("config.ini");
    // Aspect Ratio Values
    preserveYAxis = config.ReadBoolean("AspectRatio", "PreserveYAxis", false);
    // FOV Config Values
    useCustomFOV = config.ReadBoolean("FieldOfView", "useCustomFOV", false);
    explorationFOV = config.ReadInteger("FieldOfView", "explorationFOV", 70);
    combatFOV = config.ReadInteger("FieldOfView", "combatFOV", 60);
    // Framerate/VSync Config Values
    useCustomFPSCap = config.ReadBoolean("Experimental", "forceFPSCap", true);
    tMaxFPS = config.ReadInteger("Experimental", "maxFPS", 1000);
}

void fovOffsetCalc()
{
    // Declare the vertical and horizontal resolution variables.
    hRes = *(int*)((intptr_t)baseModule + hResPointer); // Grabs Horizontal Resolution integer.
    vRes = *(int*)((intptr_t)baseModule + vResPointer); // Grabs Vertical Resolution integer.

    // Convert the int values to floats, so then we can determine the aspect ratio.
    aspectRatio = (float)hRes / (float)vRes;

    if (!preserveYAxis)
    {
        // If the aspect ratio is less than or equal to 16:9, use the standard Vert- offset, so 16:10 and 4:3 resolutions will look okay.
        if (aspectRatio <= (16.0f / 9.0f))
        {
            FOVOffset = originalFOV;
        }
        // If the aspect ratio is greater than 16:9, then adjust the FOV offset based on the current screen resolution, as you don't want Vert- behavior in 21:9/32:9/48:9/etc.
        else if (aspectRatio > (16.0f / 9.0f))
        {
            // If useCustomFOV is set to "0", then calculate the vertical FOV using the new aspect ratio, the old aspect ratio, and the original FOV.
            FOVOffset = std::round((2.0f * atan(((aspectRatio) / (16.0f / 9.0f)) * tan((originalFOV * 10000.0f) / 2.0f * ((float)M_PI / 180.0f)))) * (180.0f / (float)M_PI) * 100.0f) / 100.0f / 10000.0f;
        }
    }
    else
    {
        FOVOffset = std::round((2.0f * atan(((aspectRatio) / (16.0f / 9.0f)) * tan((originalFOV * 10000.0f) / 2.0f * ((float)M_PI / 180.0f)))) * (180.0f / (float)M_PI) * 100.0f) / 100.0f / 10000.0f;
    }

    // Writes FOV offset to Memory.
    *(float*)((intptr_t)baseModule + fovOffsetPointer) = FOVOffset;
}

void fovCalc()
{
    // Instead of modifying the FOV offset, we are just going to modify the FOV offset directly.
    if (useCustomFOV)
    {

    }
    else
    {
        // Adjust Combat and Exploration FOV here. This will need to be patched in once I can fix the fucking awful typecast crashes.
        //if (*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x270) = 70.0f)
        //{
            //*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x270) = (float)110;
        //}
        //if (*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x290) = 80.0f)
        //{
            //*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x290) = (float)160;
        //}
    }
}

void uncapFPS() //Uncaps the framerate.
{
	//Writes the new t.MaxFPS cap to memory, alongside pointer.
	*(float*)(*((intptr_t*)((intptr_t)baseModule + fpsCapPointer)) + 0x0) = (float)tMaxFPS;
}

void pillarboxRemoval() // Adjusts pillarboxes based on the current aspect ratio. Probably requires exiting to the main menu to apply the static ones.
{
    // Writes pillarbox removal into memory ("F6 41 30 01" to "F6 41 30 00"). Seemingly, instead of 2C, the byte is 30.
    memcpy((LPVOID)((intptr_t)baseModule + 0x233B155), "\xF6\x41\x30\x00", 4);
}

void resolutionCheck()
{
    if (aspectRatio != (*(int*)((intptr_t)baseModule + 0x455A4F0) / *(int*)((intptr_t)baseModule + 0x455A4F4)))
    {
        fovOffsetCalc();
    }
}

void framerateCheck()
{
	if (tMaxFPS != *(float*)(*((intptr_t*)((intptr_t)baseModule + 0x04C40A58)) + 0x0))
	{
		uncapFPS();
	}
}

void StartPatch()
{
    // Reads the "config.ini" config file for values that we are going to want to modify.
    readConfig();

	// Unprotects the main module handle.
	ScopedUnprotect::FullModule UnProtect(baseModule);

    Sleep(5000); // Sleeps the thread for five seconds before applying the memory values.

	fovOffsetCalc(); // Calculates the new vertical FOV.

    fovCalc();

    pillarboxRemoval(); // Removes the in-game pillarboxing.

    // checks if CustomFPS cap is enabled before choosing which processes to loop, and if to uncap the framerate. This is done to save on CPU resources.
    if (useCustomFPSCap == true)
    {
        uncapFPS(); //Uncaps the framerate.
        float sleepinMs = ((1 / (float)tMaxFPS) * 1000); // Takes the max framerate cap and uses that for the sleep value for now, until I can get a Waitable Timer implemented.
        // Runs resolution and framerate check in a loop.
		while (check != false)
		{
            Sleep(sleepinMs);
			resolutionCheck();
			framerateCheck();
		}
    }
    else
    {
        // Runs resolution check in a loop.
		while (check != false)
		{
            float sleepinMs = ((1 / *(float*)(*((intptr_t*)((intptr_t)baseModule + 0x04C40A58)) + 0x0)) * 1000);
            Sleep(sleepinMs);
			resolutionCheck();
		}
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) // This code runs when the DLL file has been attached to the game process.
    {
		HANDLE patchThread; // Creates a handle to the patch thread, so it can be closed easier
		patchThread = (HANDLE)_beginthreadex(NULL, 0, (unsigned(__stdcall*)(void*))StartPatch, NULL, 0, 0); // Calls the StartPatch function in a new thread on start.
		// CloseHandle(patchThread); // Closes the StartPatch thread handle.
    }
    return TRUE;
}
