/* LGPL-2.1-or-later. Checks that the VC14 runtime builtins report a file
 * version (0160 adds the missing ones, vcruntime140 and msvcp140_1).
 * Loads vcruntime140, vcruntime140_1, msvcp140, msvcp140_1 and msvcp140_2 and
 * reads each loaded file's version with GetFileVersionInfo/VerQueryValue, as a
 * program checking its runtime does. Every one must report 14.x or later.
 * Run it in a fresh prefix with no native VC++ runtime, so the builtins load;
 * it prints the loaded path, but does not check that it is a builtin. Build:
 * x86_64-w64-mingw32-gcc -O1 -o vc14-version-probe.exe vc14-version-probe.c -lversion
 * (i686-w64-mingw32-gcc for the 32-bit builtins). */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( void )
{
    static const char *names[] = { "vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll",
                                   "msvcp140_1.dll", "msvcp140_2.dll" };
    int i, failures = 0;

    for (i = 0; i < (int)(sizeof(names) / sizeof(names[0])); i++)
    {
        char path[MAX_PATH] = "";
        VS_FIXEDFILEINFO *info = NULL;
        HMODULE module = LoadLibraryA( names[i] );
        DWORD size, dummy;
        void *data = NULL;
        UINT len;

        if (sizeof(void *) == 4 && !strcmp( names[i], "vcruntime140_1.dll" )) continue;  /* x64 only */
        if (!module)
        {
            printf( "FAIL: %s not loaded\n", names[i] );
            failures++;
            continue;
        }
        GetModuleFileNameA( module, path, sizeof(path) );
        size = GetFileVersionInfoSizeA( path, &dummy );
        if (size) data = malloc( size );
        if (!data || !GetFileVersionInfoA( path, 0, size, data ) ||
            !VerQueryValueA( data, "\\", (void **)&info, &len ) || len < sizeof(*info))
        {
            printf( "FAIL: %s at %s has no version\n", names[i], path );
            failures++;
        }
        else
        {
            printf( "%s: %s at %s is %u.%u.%u.%u\n", HIWORD(info->dwFileVersionMS) >= 14 ? "PASS" : "FAIL",
                    names[i], path, HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
                    HIWORD(info->dwFileVersionLS), LOWORD(info->dwFileVersionLS) );
            if (HIWORD(info->dwFileVersionMS) < 14) failures++;
        }
        free( data );
    }
    printf( failures ? "RESULT FAIL (%d)\n" : "RESULT PASS\n", failures );
    return failures != 0;
}
