/* LGPL-2.1-or-later. Checks app package lookups (0132) in a prefix booted with
 * the patched wine.inf. GetPackagesByPackageFamily() must list the VCLibs
 * framework package for this process's architecture, PackageIdFromFullName() and
 * PackageFullNameFromId() must round-trip its full name, and GetPackagePath() must
 * answer the system directory of its architecture (syswow64 for a WoW64 process,
 * as wine.inf registers it). The old stub answers zero packages, and
 * GetPackagePath() and PackageFullNameFromId() are missing. */
#include <windows.h>
#include <appmodel.h>
#include <stdio.h>
#include <wchar.h>

static int failures;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); ++failures; } } while (0)

typedef LONG (WINAPI *by_family_fn)(const WCHAR *, UINT32 *, WCHAR **, UINT32 *, WCHAR *);
typedef LONG (WINAPI *path_fn)(const PACKAGE_ID *, const UINT32, UINT32 *, WCHAR *);
typedef LONG (WINAPI *full_name_fn)(const PACKAGE_ID *, UINT32 *, WCHAR *);
typedef LONG (WINAPI *id_fn)(const WCHAR *, UINT32, UINT32 *, BYTE *);

int main(void)
{
    HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
    by_family_fn by_family = (by_family_fn)(void *)GetProcAddress(kernel32, "GetPackagesByPackageFamily");
    path_fn get_path = (path_fn)(void *)GetProcAddress(kernel32, "GetPackagePath");
    full_name_fn full_name = (full_name_fn)(void *)GetProcAddress(kernel32, "PackageFullNameFromId");
    id_fn id_from_name = (id_fn)(void *)GetProcAddress(kernel32, "PackageIdFromFullName");
    const WCHAR *arch = sizeof(void *) == 8 ? L"_x64_" : L"_x86_";
    WCHAR *names[8], buffer[1024], rebuilt[256], path[MAX_PATH], sysdir[MAX_PATH];
    UINT32 count = 8, length = ARRAYSIZE(buffer), i, size;
    BYTE id_buffer[1024];
    const WCHAR *mine = NULL;
    LONG ret;

    setvbuf(stdout, NULL, _IONBF, 0);
    CHECK(by_family && id_from_name, "GetPackagesByPackageFamily and PackageIdFromFullName exported");
    CHECK(get_path != NULL, "GetPackagePath exported");
    CHECK(full_name != NULL, "PackageFullNameFromId exported");
    if (!by_family || !id_from_name) return 1;

    ret = by_family(L"Microsoft.VCLibs.140.00_8wekyb3d8bbwe", &count, names, &length, buffer);
    printf("GetPackagesByPackageFamily ret=%ld count=%u\n", ret, count);
    CHECK(ret == ERROR_SUCCESS && count >= 1, "VCLibs framework package is listed");
    for (i = 0; ret == ERROR_SUCCESS && i < count && i < 8; i++)
    {
        printf("  %ls\n", names[i]);
        if (wcsstr(names[i], arch)) mine = names[i];
    }
    CHECK(mine != NULL, "a package for this architecture is listed");
    if (!mine || !get_path || !full_name) goto done;

    size = sizeof(id_buffer);
    ret = id_from_name(mine, 0, &size, id_buffer);
    CHECK(ret == ERROR_SUCCESS, "PackageIdFromFullName");
    if (ret) goto done;

    length = ARRAYSIZE(rebuilt);
    ret = full_name((PACKAGE_ID *)id_buffer, &length, rebuilt);
    CHECK(ret == ERROR_SUCCESS && !wcscmp(rebuilt, mine), "PackageFullNameFromId round-trips the full name");

    length = ARRAYSIZE(path);
    ret = get_path((PACKAGE_ID *)id_buffer, 0, &length, path);
    {
        BOOL wow64 = FALSE;
        IsWow64Process(GetCurrentProcess(), &wow64);
        if (wow64) GetSystemWow64DirectoryW(sysdir, ARRAYSIZE(sysdir));
        else GetSystemDirectoryW(sysdir, ARRAYSIZE(sysdir));
    }
    printf("GetPackagePath ret=%ld path=%ls system=%ls\n", ret, ret ? L"" : path, sysdir);
    CHECK(ret == ERROR_SUCCESS && !_wcsicmp(path, sysdir), "GetPackagePath answers the system directory");

done:
    printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
    return failures ? 1 : 0;
}
