/* LGPL-2.1-or-later. Checks filtered CredEnumerateW() against registry credentials (0125).
 * A generic credential is written under a unique prefix (Wine keeps generic
 * credentials in the registry, not in the macOS Keychain), so a filter for that
 * prefix matches the registry but nothing in the host store. It must return the
 * credential; a filter matching nothing must fail with ERROR_NOT_FOUND. */
#include <windows.h>
#include <wincred.h>
#include <stdio.h>

static int failures;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d, error %lu)\n", label, __LINE__, GetLastError()); ++failures; } } while (0)

int main(void)
{
    WCHAR target[96], filter[96], none[96];
    CREDENTIALW cred = {0}, **list = NULL;
    BYTE blob[] = {'p', 'r', 'o', 'b', 'e'};
    DWORD count = 0, pid = GetCurrentProcessId();
    BOOL ok;

    setvbuf(stdout, NULL, _IONBF, 0);
    swprintf(target, ARRAYSIZE(target), L"RuneonCredProbe|%lu|DeviceKey", pid);
    swprintf(filter, ARRAYSIZE(filter), L"RuneonCredProbe|%lu|*", pid);
    swprintf(none, ARRAYSIZE(none), L"RuneonCredProbeNone|%lu|*", pid);

    cred.Type = CRED_TYPE_GENERIC;
    cred.TargetName = target;
    cred.CredentialBlobSize = sizeof(blob);
    cred.CredentialBlob = blob;
    cred.Persist = CRED_PERSIST_LOCAL_MACHINE;
    CHECK(CredWriteW(&cred, 0), "CredWriteW generic credential");

    ok = CredEnumerateW(filter, 0, &count, &list);
    CHECK(ok, "filtered CredEnumerateW finds the registry credential");
    if (ok)
    {
        CHECK(count == 1, "exactly one match");
        CHECK(count >= 1 && !wcscmp(list[0]->TargetName, target), "matching target name");
        CHECK(count >= 1 && list[0]->CredentialBlobSize == sizeof(blob)
              && !memcmp(list[0]->CredentialBlob, blob, sizeof(blob)), "matching blob");
        CredFree(list);
    }

    SetLastError(0xdeadbeef);
    count = 1;
    ok = CredEnumerateW(none, 0, &count, &list);
    CHECK(!ok && GetLastError() == ERROR_NOT_FOUND, "a filter matching nothing fails with ERROR_NOT_FOUND");
    if (ok) CredFree(list);

    CHECK(CredDeleteW(target, CRED_TYPE_GENERIC, 0), "CredDeleteW cleanup");
    printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
    return failures ? 1 : 0;
}
