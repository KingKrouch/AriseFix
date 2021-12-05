#include "../../Source/pch.h"
#include "wtypes.h"
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <process.h>
#include <stdio.h>

#include "Arise.h"
// Silent's ModUtils for unprotecting the main module, pattern scanning, and inserting trampolines.
#include "../../Source/ThirdParty/ModUtils/MemoryMgr.h"
#include "../../Source/ThirdParty/ModUtils/Patterns.h"
#include "../../Source/ThirdParty/ModUtils/Trampoline.h"
// ThirteenAG's IniReader for reading config files.
#include "../../Source/ThirdParty/IniReader/IniReader.h"

using namespace std;
using namespace hook;
using namespace Memory::VP;

// Process HMODULE variable
HMODULE baseModule = GetModuleHandle(NULL);

namespace config
{
    // Config File ----------------
    CIniReader config("config.ini"); // The name of the INI file that will be read from.
    // Config Variables -----------
    bool preserveYAxis;
    int combatFOV;
    int explorationFOV;
    bool useCustomFOV;
    bool useCustomFPSCap;
    int maxFPS;
    // Functions ------------------
    void readConfig() // Reads variables from the config.ini, and then sets their equivalent values.
    {
        // Aspect Ratio Values
        preserveYAxis = config.ReadBoolean("AspectRatio", "PreserveYAxis", false);
        // FOV Config Values
        useCustomFOV = config.ReadBoolean("FieldOfView", "useCustomFOV", false);
        explorationFOV = config.ReadInteger("FieldOfView", "explorationFOV", 70);
        combatFOV = config.ReadInteger("FieldOfView", "combatFOV", 60);
        // Framerate/VSync Config Values
        useCustomFPSCap = config.ReadBoolean("Experimental", "forceFPSCap", true);
        maxFPS = config.ReadInteger("Experimental", "maxFPS", 1000);
    }

    void configInit() // Initializes configuration file reading and variables.
    {
        cout.flush();
        //freopen(FILE**)stdout, "CONOUT$", "w", stdout); // Allows us to add outputs to the ASI Loader Console Window.
        cout.clear();
        cin.clear();
        readConfig();
    }
}

namespace resolution
{
    // Resolution and Aspect Ratio variables -------------
    int hRes;
    int vRes;
    float aspectRatio;
    // Namespaces ----------------------------------------

    // Functions -----------------------------------------
    void pillarboxRemoval() // Removes pillarboxes altogether.
    {
        // Writes pillarbox removal into memory ("F6 41 30 01" to "F6 41 30 00"). Seemingly, instead of 2C, the byte is 30.
        auto pillarboxRemoval = get_pattern("f6 41 ? ? 41 0f 29 7b", 3); // Check a pattern for pillarboxing, and we pick the fourth byte to patch, usually being "01"
        Patch<bool>(pillarboxRemoval, false);
    }

    void aspectRatioCalc()
    {
        // Grabs the opcode that is called when the resolution changes, so we can grab the current resolution
        auto HresChange = get_pattern("44 89 25 ? ? ? ? 8d 42", 4);
        auto VresChange = get_pattern("44 89 3d ? ? ? ? 44 89 35", 4);
        ReadOffsetValue<int>(HresChange, hRes);
        ReadOffsetValue<int>(VresChange, vRes);

        // Convert the int values to floats, so then we can determine the aspect ratio.
        aspectRatio = (float)hRes / (float)vRes;
    }

    void resolutionInit() // Initializes resolution related stuff.
    {
        pillarboxRemoval();
    }

}

namespace fov
{
    // FOV variables -------------------------------------------------------------
    float originalFOV = 0.008726646192; // Declares the original 16:9 vertical FOV.
    float FOVOffset;
    // Namespaces ----------------------------------------------------------------
    using namespace resolution;
    using namespace config;
    // Functions -----------------------------------------------------------------
    void fovPatch()
    {
        // Adjust Combat and Exploration FOV here. This will need to be patched in once I can fix the fucking awful typecast crashes.
        //if (*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x270) = 70.0f) // Exploration FOV
        //{
            //*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x270) = (float)110;
        //}
        //if (*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x290) = 60.0f) // Combat FOV
        //{
            //*(float*)(*(((intptr_t*)((intptr_t*)(((intptr_t)baseModule + 0x04C89838))) + 0x250)) + 0x290) = (float)100;
        //}
    }
    void fovOffsetCalc()
    {
        aspectRatioCalc(); // Calculates the new aspect ratio before making any changes to FOV offsets.

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
        auto fovOffsetFunc = get_pattern("f3 0f 59 05 ? ? ? ? e8 ? ? ? ? f3 0f 10 3d", 4);
        WriteOffsetValue(fovOffsetFunc, &FOVOffset);
        return;
    }

    void fovInit()
    {
        fovOffsetCalc();
    }
}

namespace ui
{
    // Resolution and Aspect Ratio variables -------------
    float horScale;
    float horPos;
    float verScale;
    float verPos;
    // Namespaces -----------------
    using namespace resolution;
    // Functions ------------------
    void updateOffsets() // Will be called whenever the in-game resolution is changed, so UI offsets and scaling is adjusted accordingly.
    {
        aspectRatioCalc(); // Calculates the new aspect ratio before making any changes to UI scale and position.

        if (aspectRatio < (1920 / 1080)) // If the aspect ratio is narrower than 16:9, then calculate the vertical UI offset and scale
        {
            horScale = 1.0f;
            horPos = 0.0f;
            verScale = (((float)hRes / vRes) / (1920 / 1080));
            verPos = floor(((1080 / ((1920 / 1080) / ((float)hRes / vRes))) - 1080) / 2); // (1080p equivalent of current screen aspect ratio - 1080) / 2
        }
        if (aspectRatio > (1920 / 1080)) // If the aspect ratio is wider than 16:9, then calculate the horizontal UI offset and scale
        {
            horScale = ((1920 / 1080) / ((float)hRes / vRes)); // 1080p aspect ratio divided by screen aspect ratio
            horPos = floor(((1920 / ((1920 / 1080) / ((float)hRes / vRes))) - 1920) / 2); // (1080p equivalent of current screen aspect ratio - 1920) / 2
            verScale = 1.0f;
            verPos = 0.0f;
        }
        if (aspectRatio == (1920 / 1080)) // If the aspect ratio is 16:9, keep the horizontal and vertical UI offset/scale at their defaults
        {
            horScale = 1.0f;
            horPos = 0.0f;
            verScale = 1.0f;
            verPos = 0.0f;
        }
        return;
    }

    void initOffsets() // Sets up the UI offset and scale functions to use our own variables that we can modify on the fly.
    {
        try
        {
            // Horizontal and Vertical UI Scaling Patterns -----------------------
            auto horUIScaleFunc_pattern = pattern("f3 0f 10 73 ? 45 0f 57 c9");
            auto horUIScaleFunc_patternJmp = pattern("45 0f 57 c9 f3 44 0f 10 53");
            auto verUIScaleFunc_pattern = pattern("f3 44 0f 10 53 ? f3 0f 11 b5");
            auto verUIScaleFunc_patternJmp = pattern("f3 0f 11 b5 ? ? ? ? f3 44 0f 11 95 ? ? ? ? 83 e9");
            // Horizontal and Vertical UI Positional Patterns --------------------
            auto horUIPosFunc_pattern = pattern("f3 44 0f 58 43 ? f3 44 0f 58 4b ? 48 8b 4b");
            auto horUIPosFunc_patternJmp = pattern("f3 44 0f 58 4b ? 48 8b 4b");
            auto verUIPosFunc_pattern = pattern("f3 44 0f 58 4b ? 48 8b 4b");
            auto verUIPosFunc_patternJmp = pattern("48 8b 4b ? 48 81 c1 ? ? ? ? c7 45");

            updateOffsets();
        }
        TXN_CATCH();
    }

    void updateRes()
    {
        // Namespaces ----------------------------------------------------------------------------------
        using namespace fov;
        // Functions -----------------------------------------------------------------------------------
        updateOffsets();
        fovOffsetCalc();
    }

    void resolutionHooks() // Adds hooks for FOV offset and UI offset/scale calculation to when the game resolution changes.
    {
        Trampoline* resHookTrampoline = Trampoline::MakeTrampoline(baseModule);
        auto viewportAdjust = get_pattern("48 89 5c 24 ? 48 89 6c 24 ? 48 89 74 24 ? 57 48 83 ec ? 8b 1d");
        InjectHook(viewportAdjust, resHookTrampoline->Jump(updateRes), PATCH_CALL);
    }

    void UiInit()
    {
        resolutionHooks();
        initOffsets();
    }
}

namespace framerate
{
    // Namespaces -----------------
    using namespace config;
    // Functions ------------------
    void uncapFPS() //Uncaps the framerate by modifying the t.MaxFPS opcode to point to our own framerate float value.
    {
        if (useCustomFPSCap == true)
        {
            try
            {
                // We only want to detour the function on the first five bytes, which is the opcode that accesses the register containing the framerate cap.
                auto fpsCapFunc_pattern = pattern("f3 0f 10 34 9f 41 0f 2e f1 74 ? 0f 28 c6");
                // Afterwards, we then jump back to the part after the opcode is written to.
                auto fpsCapFunc_patternJmp = pattern("41 0f 2e f1 74 ? 0f 28 c6");

                if (fpsCapFunc_pattern.count(1).size() == 1 && fpsCapFunc_patternJmp.count(1).size() == 1)
                {
                    auto fpsCapFunc = fpsCapFunc_pattern.get_first();
                    auto fpsCapFuncJmp = fpsCapFunc_patternJmp.get_first();

                    Trampoline* fpsCapTrampoline = Trampoline::MakeTrampoline(fpsCapFunc);

                    float* maxFPSptr = fpsCapTrampoline->Pointer<float>();
                    *maxFPSptr = (float)maxFPS;

                    // Creates a space that we can use to make assembly functions using binary representations of them.
                    const uint8_t fpsCapPayload[] = {
                        //0xF3, 0x0F, 0x10, 0x34, 0x9F, // movss xmm6,[rdi+rbx*4] (original FPS cap instruction)
                          0xF3, 0x0F, 0x10, 0x35, 0x00, 0x00, 0x00, 0x00, // movss xmm6,[maxFPSptr] (Add 1 onto 0x34 to stop the crashing)
                          0xE9, 0x00, 0x00, 0x00, 0x00, // jmp fpsCapFuncJmp+6 (Add a byte to the count of the original FPS cap instruction to go to the next instruction)
                    };

                    std::byte* space = fpsCapTrampoline->RawSpace(sizeof(fpsCapPayload)); // Grabs the space that the payload takes.
                    memcpy(space, fpsCapPayload, sizeof(fpsCapPayload)); // Creates a trampoline the size of our FPS cap payload.

                    WriteOffsetValue(space + 4, maxFPSptr); // Start the offset for the pointer reference on the fifth byte of the new FPS cap instruction
                    WriteOffsetValue(space + 8 + 1, reinterpret_cast<intptr_t>(fpsCapFuncJmp)); // Place the JMP address on the tenth byte in our code cave.

                    InjectHook(fpsCapFunc, space, PATCH_JUMP); // Inject a hook into the function and redirect it to the ASM function that we want to modify.
                }
            }
            TXN_CATCH();
        }
        return;
    }

    void framerateInit()
    {
        uncapFPS();
    }
}

void resolutionFunctions()
{
    using namespace fov;
    using namespace ui;

    fovOffsetCalc();
}

void resolutionDetours()
{
    // Grabs the opcode that is called after the vertical resolution changes
    auto resChangeFunc = get_pattern("48 63 f0 85 c0 78 ? 48 8b fe 48 c1 e7 ? 0f 1f 84 00");
    // We are then going to detour it to do some FOV offset calculations and UI Calculations.
    InjectHook(resChangeFunc, resolutionFunctions);
}

void AriseFix::StartPatch()
{
    // Namespaces ----------------------------------------------------------------------------------
    using namespace config;
    using namespace framerate;
    using namespace resolution;
    using namespace fov;
    using namespace ui;
    // Functions -----------------------------------------------------------------------------------
    configInit(); // Reads the "config.ini" config file for values that we are going to want to modify.
    ScopedUnprotect::FullModule UnProtect(baseModule); // Unprotects the main module handle.

    // Initializes Patching Functionality ----------------------------------------------------------
    resolutionInit(); // Initializes the resolution patch functionality.
    framerateInit(); // Initializes the framerate uncap functionality.
    UiInit(); // Initializes the UI functionality.
    fovInit(); // Initializes the FOV functionality.
}