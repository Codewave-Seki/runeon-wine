/* LGPL-2.1-or-later. HKCU Wow6432Node probe for 0183.
 * On Windows HKCU\Software is shared between the 32-bit and 64-bit registry
 * views; only listed Software\Classes subtrees such as CLSID are redirected.
 * Wine redirected the 32-bit view of everything under HKCU\Software into
 * HKCU\Software\Wow6432Node as soon as that key existed. The probe creates
 * that key if it is missing, writes distinct sentinels to
 * HKCU\Software\RuneonWow64Probe\Key (0x64) and to the same path under
 * Software\Wow6432Node (0x32), and checks, in the default view and with
 * KEY_WOW64_32KEY and KEY_WOW64_64KEY:
 *   - a full path and a path continued from an open Software or
 *     RuneonWow64Probe handle open the shared key;
 *   - a path that names Wow6432Node opens the Wow6432Node key;
 *   - a deep key created and a value written through the 32-bit view land in
 *     the shared key, and RegGetValueA with RRF_SUBKEY_WOW6432KEY reads it;
 *   - HKLM\Software, HKLM\Software\Classes (shared) and
 *     HKLM\Software\Classes\CLSID (redirected) still behave as on Windows;
 *   - HKCU\Software\Classes\CLSID is still redirected when
 *     HKCU\Software\Classes\Wow6432Node exists.
 * Prints one line per check and exits 0 if all pass, 1 if any fails, 2 on a
 * setup error. It removes its keys, and the HKCU keys above them only if it
 * created them and they are empty. Run it only in a scratch prefix, never
 * against a real user prefix or Steam bottle: it writes to HKCU and HKLM.
 * Needs write access to HKLM (a Wine prefix has it). Build both and run
 * both in the same prefix:
 *   i686-w64-mingw32-gcc -O2 -Wall -Werror \
 *     -o hkcu-wow6432node-probe32.exe hkcu-wow6432node-probe.c
 *   x86_64-w64-mingw32-gcc -O2 -Wall -Werror \
 *     -o hkcu-wow6432node-probe64.exe hkcu-wow6432node-probe.c
 * It does not cover HKEY_USERS paths other than the current user, renaming
 * keys, registry notifications, or Windows itself. */
#include <windows.h>
#include <stdio.h>

#ifndef RRF_SUBKEY_WOW6432KEY
#define RRF_SUBKEY_WOW6432KEY 0x00020000
#endif

#define PROBE "RuneonWow64Probe"
#define CLSID_PROBE "CLSID\\{5e8f2f4b-1f3a-4c0e-9a51-72d0c0b1a072}"

static const DWORD views[] = { 0, KEY_WOW64_32KEY, KEY_WOW64_64KEY };
static const BOOL is_64bit = sizeof(void *) > 4;
static int failures;

static const char *view_name(DWORD view)
{
    return view == KEY_WOW64_32KEY ? "32KEY" : view == KEY_WOW64_64KEY ? "64KEY" : "default";
}

/* 0 when the key or value is missing, so a missing key never passes as a sentinel */
static DWORD read_value(HKEY root, const char *path, DWORD view, const char *value)
{
    DWORD data = 0, size = sizeof(data);
    HKEY key;

    if (RegOpenKeyExA(root, path, 0, KEY_QUERY_VALUE | view, &key)) return 0;
    if (RegQueryValueExA(key, value, NULL, NULL, (BYTE *)&data, &size)) data = 0;
    RegCloseKey(key);
    return data;
}

static void check(const char *what, DWORD view, DWORD got, DWORD expect)
{
    printf("%-4s %-58s %-7s got 0x%lx expect 0x%lx\n", got == expect ? "ok" : "FAIL", what,
           view_name(view), (unsigned long)got, (unsigned long)expect);
    if (got != expect) failures++;
}

static BOOL write_value(HKEY root, const char *path, DWORD view, const char *value, DWORD data, DWORD *disp)
{
    HKEY key;
    LONG ret;

    if (RegCreateKeyExA(root, path, 0, NULL, 0, KEY_ALL_ACCESS | view, NULL, &key, disp)) return FALSE;
    ret = RegSetValueExA(key, value, 0, REG_DWORD, (const BYTE *)&data, sizeof(data));
    RegCloseKey(key);
    return !ret;
}

/* deletes path and its subkeys in the 64-bit view, where nothing is redirected */
static BOOL delete_tree(HKEY root, const char *path)
{
    char name[MAX_PATH], sub[2 * MAX_PATH];
    HKEY key;

    if (RegOpenKeyExA(root, path, 0, KEY_ENUMERATE_SUB_KEYS | KEY_WOW64_64KEY, &key)) return FALSE;
    while (!RegEnumKeyA(key, 0, name, sizeof(name)))
    {
        snprintf(sub, sizeof(sub), "%s\\%s", path, name);
        if (!delete_tree(root, sub)) break;
    }
    RegCloseKey(key);
    return !RegDeleteKeyExA(root, path, KEY_WOW64_64KEY, 0);
}

static void delete_if_created_and_empty(HKEY root, const char *path, DWORD disp)
{
    DWORD subkeys = 1, values = 1;
    HKEY key;

    if (disp != REG_CREATED_NEW_KEY) return;
    if (RegOpenKeyExA(root, path, 0, KEY_READ | KEY_WOW64_64KEY, &key)) return;
    RegQueryInfoKeyA(key, NULL, NULL, NULL, &subkeys, NULL, NULL, &values, NULL, NULL, NULL, NULL);
    RegCloseKey(key);
    if (!subkeys && !values) RegDeleteKeyExA(root, path, KEY_WOW64_64KEY, 0);
}

/* HKCU keys the Classes checks may create in a fresh prefix, parents first */
static const char *const classes_parents[] = {
    "Software\\Classes", "Software\\Classes\\CLSID", "Software\\Classes\\Wow6432Node", "Software\\Classes\\Wow6432Node\\CLSID",
};
static DWORD classes_parent_disp[ARRAYSIZE(classes_parents)];

static void cleanup(DWORD wow_disp)
{
    int n;

    delete_tree(HKEY_CURRENT_USER, "Software\\" PROBE);
    delete_tree(HKEY_CURRENT_USER, "Software\\Wow6432Node\\" PROBE);
    delete_tree(HKEY_CURRENT_USER, "Software\\Classes\\" CLSID_PROBE);
    delete_tree(HKEY_CURRENT_USER, "Software\\Classes\\Wow6432Node\\" CLSID_PROBE);
    delete_tree(HKEY_LOCAL_MACHINE, "Software\\" PROBE);
    delete_tree(HKEY_LOCAL_MACHINE, "Software\\Wow6432Node\\" PROBE);
    delete_tree(HKEY_LOCAL_MACHINE, "Software\\Classes\\" PROBE);
    delete_tree(HKEY_LOCAL_MACHINE, "Software\\Classes\\" CLSID_PROBE);
    delete_tree(HKEY_LOCAL_MACHINE, "Software\\Classes\\Wow6432Node\\" CLSID_PROBE);
    for (n = ARRAYSIZE(classes_parents) - 1; n >= 0; n--)
        delete_if_created_and_empty(HKEY_CURRENT_USER, classes_parents[n], classes_parent_disp[n]);
    delete_if_created_and_empty(HKEY_CURRENT_USER, "Software\\Wow6432Node", wow_disp);
}

int main(void)
{
    DWORD wow_disp = 0, data, size, i;
    char what[160];
    HKEY key;

    printf("%d-bit process\n", is_64bit ? 64 : 32);
    /* the trigger: HKCU\Software\Wow6432Node exists */
    if (RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Wow6432Node", 0, NULL, 0, KEY_READ | KEY_WOW64_64KEY,
                        NULL, &key, &wow_disp))
    {
        printf("setup failed: cannot create HKCU\\Software\\Wow6432Node\n");
        return 2;
    }
    RegCloseKey(key);
    if (!write_value(HKEY_CURRENT_USER, "Software\\Wow6432Node\\" PROBE "\\Key", KEY_WOW64_64KEY, "sentinel", 0x32, NULL)
        || !write_value(HKEY_CURRENT_USER, "Software\\" PROBE "\\Key", KEY_WOW64_64KEY, "sentinel", 0x64, NULL))
    {
        printf("setup failed: cannot write the sentinels\n");
        cleanup(wow_disp);
        return 2;
    }

    for (i = 0; i < ARRAYSIZE(views); i++)
    {
        check("HKCU\\Software\\" PROBE "\\Key", views[i],
              read_value(HKEY_CURRENT_USER, "Software\\" PROBE "\\Key", views[i], "sentinel"), 0x64);
        check("HKCU\\Software\\Wow6432Node\\" PROBE "\\Key", views[i],
              read_value(HKEY_CURRENT_USER, "Software\\Wow6432Node\\" PROBE "\\Key", views[i], "sentinel"), 0x32);

        if (!RegOpenKeyExA(HKEY_CURRENT_USER, "Software", 0, KEY_READ | views[i], &key))
        {
            check("Software handle + " PROBE "\\Key", views[i], read_value(key, PROBE "\\Key", views[i], "sentinel"), 0x64);
            check("Software handle + Wow6432Node\\" PROBE "\\Key", views[i],
                  read_value(key, "Wow6432Node\\" PROBE "\\Key", views[i], "sentinel"), 0x32);
            RegCloseKey(key);
        }
        else check("open HKCU\\Software", views[i], 0, 1);

        if (!RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\" PROBE, 0, KEY_READ | views[i], &key))
        {
            check(PROBE " handle + Key", views[i], read_value(key, "Key", views[i], "sentinel"), 0x64);
            RegCloseKey(key);
        }
        else check("open HKCU\\Software\\" PROBE, views[i], 0, 1);
    }

    /* a deep key whose parents do not exist yet goes through the manual walk in kernelbase */
    if (!write_value(HKEY_CURRENT_USER, "Software\\" PROBE "\\Created\\Deep", KEY_WOW64_32KEY, "sentinel", 0x33, NULL))
        check("create Created\\Deep through 32KEY", KEY_WOW64_32KEY, 0, 1);
    check("32KEY-created " PROBE "\\Created\\Deep", KEY_WOW64_64KEY,
          read_value(HKEY_CURRENT_USER, "Software\\" PROBE "\\Created\\Deep", KEY_WOW64_64KEY, "sentinel"), 0x33);
    check("32KEY-created Wow6432Node\\" PROBE "\\Created\\Deep", KEY_WOW64_64KEY,
          read_value(HKEY_CURRENT_USER, "Software\\Wow6432Node\\" PROBE "\\Created\\Deep", KEY_WOW64_64KEY, "sentinel"), 0);

    for (i = 0; i < 2; i++)
    {
        DWORD view = i ? KEY_WOW64_32KEY : 0;
        snprintf(what, sizeof(what), "value written through %s to " PROBE "\\Key", view_name(view));
        if (!write_value(HKEY_CURRENT_USER, "Software\\" PROBE "\\Key", view, "written", 0x100 + i, NULL))
            check(what, view, 0, 1);
        check(what, KEY_WOW64_64KEY,
              read_value(HKEY_CURRENT_USER, "Software\\" PROBE "\\Key", KEY_WOW64_64KEY, "written"), 0x100 + i);
    }

    data = 0;
    size = sizeof(data);
    RegGetValueA(HKEY_CURRENT_USER, "Software\\" PROBE "\\Key", "sentinel",
                 RRF_RT_REG_DWORD | RRF_SUBKEY_WOW6432KEY, NULL, &data, &size);
    check("RegGetValueA RRF_SUBKEY_WOW6432KEY " PROBE "\\Key", KEY_WOW64_32KEY, data, 0x64);

    /* HKLM\Software stays redirected, HKLM\Software\Classes shared, its CLSID redirected */
    if (!write_value(HKEY_LOCAL_MACHINE, "Software\\" PROBE, KEY_WOW64_64KEY, "sentinel", 0x64, NULL)
        || !write_value(HKEY_LOCAL_MACHINE, "Software\\" PROBE, KEY_WOW64_32KEY, "sentinel", 0x32, NULL)
        || !write_value(HKEY_LOCAL_MACHINE, "Software\\Classes\\" PROBE, KEY_WOW64_64KEY, "sentinel", 0x64, NULL)
        || !write_value(HKEY_LOCAL_MACHINE, "Software\\Classes\\" CLSID_PROBE, KEY_WOW64_64KEY, "sentinel", 0x64, NULL)
        || !write_value(HKEY_LOCAL_MACHINE, "Software\\Classes\\" CLSID_PROBE, KEY_WOW64_32KEY, "sentinel", 0x32, NULL))
        check("HKLM setup", 0, 0, 1);
    for (i = 0; i < ARRAYSIZE(views); i++)
    {
        DWORD redirected = views[i] == KEY_WOW64_32KEY || (!views[i] && !is_64bit) ? 0x32 : 0x64;
        check("HKLM\\Software\\" PROBE, views[i],
              read_value(HKEY_LOCAL_MACHINE, "Software\\" PROBE, views[i], "sentinel"), redirected);
        check("HKLM\\Software\\Classes\\" PROBE " (shared)", views[i],
              read_value(HKEY_LOCAL_MACHINE, "Software\\Classes\\" PROBE, views[i], "sentinel"), 0x64);
        check("HKLM\\Software\\Classes\\CLSID\\{probe}", views[i],
              read_value(HKEY_LOCAL_MACHINE, "Software\\Classes\\" CLSID_PROBE, views[i], "sentinel"), redirected);
    }

    /* Wine's existing HKCU\Software\Classes redirection is kept */
    for (i = 0; i < ARRAYSIZE(classes_parents); i++)
    {
        if (RegCreateKeyExA(HKEY_CURRENT_USER, classes_parents[i], 0, NULL, 0, KEY_READ | KEY_WOW64_64KEY, NULL,
                            &key, &classes_parent_disp[i]))
            continue;
        RegCloseKey(key);
    }
    if (!write_value(HKEY_CURRENT_USER, "Software\\Classes\\" CLSID_PROBE, KEY_WOW64_64KEY, "sentinel", 0x64, NULL)
        || !write_value(HKEY_CURRENT_USER, "Software\\Classes\\Wow6432Node\\" CLSID_PROBE, KEY_WOW64_64KEY, "sentinel", 0x32, NULL))
        check("HKCU classes setup", 0, 0, 1);
    for (i = 0; i < ARRAYSIZE(views); i++)
    {
        DWORD redirected = views[i] == KEY_WOW64_32KEY || (!views[i] && !is_64bit) ? 0x32 : 0x64;
        check("HKCU\\Software\\Classes\\CLSID\\{probe}", views[i],
              read_value(HKEY_CURRENT_USER, "Software\\Classes\\" CLSID_PROBE, views[i], "sentinel"), redirected);
    }

    cleanup(wow_disp);
    printf("%d failure(s)\n", failures);
    return failures ? 1 : 0;
}
