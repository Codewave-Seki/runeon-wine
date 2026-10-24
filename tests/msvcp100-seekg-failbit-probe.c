/* LGPL-2.1-or-later. Checks that msvcp100 seekg() keeps the state of a failed stream (0136).
 * Native msvcp100 leaves the state of a stream that has failbit set unchanged
 * when either seekg() overload is called. Wine's msvcp100 used to clear it, so a
 * program that seeks before testing fail(), as an inlined ifstream does after
 * a failed open, took a missing file for an open one. The probe builds a failed
 * stream from the exported basic_istream constructor (a stream without a buffer
 * starts with badbit; failbit is added), calls seekg() and checks failbit.
 * Build for i386 only: the exports are the 32-bit thiscall names.
 *   i686-w64-mingw32-gcc -O1 -Wall -Werror -o msvcp100-seekg-failbit-probe.exe msvcp100-seekg-failbit-probe.c
 * Run it with the builtin msvcp100 and with a native copy next to it
 * (WINEDLLOVERRIDES=msvcp100=n); both must print PASS. It does not cover the
 * wchar_t streams, other msvcp versions, x86_64 or seekg() on a good stream. */
#include <windows.h>
#include <stdio.h>

#define THISCALL __attribute__((thiscall))
#define IOSTATE_failbit 2

/* fpos<_Mbstatet>: 8-byte members, 24 bytes with the 32-bit MSVC alignment. */
typedef struct
{
    __int64 off __attribute__((aligned(8)));
    __int64 pos __attribute__((aligned(8)));
    int state;
} fpos_mbstatet;

typedef void *(THISCALL *istream_ctor)(void *, void *, BOOLEAN, int);
typedef void (THISCALL *istream_vbase_dtor)(void *);
typedef void (THISCALL *ios_setstate)(void *, int, BOOLEAN);
typedef void *(THISCALL *seekg_off)(void *, __int64, int);
typedef void *(THISCALL *seekg_fpos)(void *, fpos_mbstatet);
typedef int (THISCALL *ios_rdstate)(const void *);

static int failures;

static void *stream_ios(void *stream)
{
    /* The vbptr at offset 0 leads to the virtual base basic_ios, which starts with ios_base. */
    const int *vbtable = *(const int **)stream;
    return (char *)stream + vbtable[1];
}

static void check(HMODULE msvcp, BOOL use_fpos)
{
    istream_ctor ctor = (void *)GetProcAddress(msvcp,
            "??0?$basic_istream@DU?$char_traits@D@std@@@std@@QAE@PAV?$basic_streambuf@DU?$char_traits@D@std@@@1@_N@Z");
    istream_vbase_dtor dtor = (void *)GetProcAddress(msvcp, "??_D?$basic_istream@DU?$char_traits@D@std@@@std@@QAEXXZ");
    ios_setstate setstate = (void *)GetProcAddress(msvcp, "?setstate@?$basic_ios@DU?$char_traits@D@std@@@std@@QAEXH_N@Z");
    seekg_off seek_off = (void *)GetProcAddress(msvcp, "?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@_JH@Z");
    seekg_fpos seek_fpos = (void *)GetProcAddress(msvcp,
            "?seekg@?$basic_istream@DU?$char_traits@D@std@@@std@@QAEAAV12@V?$fpos@H@2@@Z");
    ios_rdstate rdstate = (void *)GetProcAddress(msvcp, "?rdstate@ios_base@std@@QBEHXZ");
    const char *label = use_fpos ? "seekg(fpos)" : "seekg(off, dir)";
    fpos_mbstatet start = {0};
    __int64 storage[64] = {0};
    void *stream = storage;
    int before, after;

    if (!ctor || !dtor || !setstate || !seek_off || !seek_fpos || !rdstate)
    {
        printf("FAIL: missing msvcp100 export\n");
        ++failures;
        return;
    }
    /* No stream buffer; the last argument constructs the virtual base. */
    ctor(stream, NULL, FALSE, 1);
    setstate(stream_ios(stream), IOSTATE_failbit, FALSE);
    before = rdstate(stream_ios(stream));
    if (use_fpos)
        seek_fpos(stream, start);
    else
        seek_off(stream, 0, 0);
    after = rdstate(stream_ios(stream));
    printf("%s: state %#x -> %#x\n", label, before, after);
    if (!(before & IOSTATE_failbit) || !(after & IOSTATE_failbit))
    {
        printf("FAIL: %s cleared failbit of a failed stream\n", label);
        ++failures;
    }
    dtor(stream);
}

int main(void)
{
    HMODULE msvcp;

    setvbuf(stdout, NULL, _IONBF, 0);
    if (!(msvcp = LoadLibraryA("msvcp100.dll")))
    {
        printf("FAIL: msvcp100.dll not loaded (error %lu)\n", GetLastError());
        return 1;
    }
    check(msvcp, FALSE);
    check(msvcp, TRUE);
    printf("%s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
