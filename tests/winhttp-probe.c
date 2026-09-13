#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <wchar.h>

int wmain(int argc, WCHAR **argv)
{
    HINTERNET session, connection, request;
    WCHAR pac_url[128], long_url[1200], header[256];
    WINHTTP_AUTOPROXY_OPTIONS options = {0};
    WINHTTP_PROXY_INFO proxy = {0};
    DWORD count, error;
    char buffer[16];
    int port, failures = 0;
    BOOL result;
    if (argc != 3) return 2;
    port = _wtoi(argv[2]);
    session = WinHttpOpen(L"Runeon backport regression", WINHTTP_ACCESS_TYPE_NO_PROXY,
                         WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return 3;
    WinHttpSetTimeouts(session, 5000, 5000, 5000, 5000);
    if (!wcscmp(argv[1], L"headers"))
    {
        connection = WinHttpConnect(session, L"127.0.0.1", port, 0);
        request = WinHttpOpenRequest(connection, L"GET", L"/headers", NULL,
                                    WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
        result = request && WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                              WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        if (result) result = WinHttpReceiveResponse(request, NULL);
        if (result)
        {
            count = sizeof(header);
            result = WinHttpQueryHeaders(request, WINHTTP_QUERY_CONNECTION, NULL, header, &count, NULL);
            if (!result || wcslen(header) != 100) failures++;
            for (unsigned i = 0; result && i < wcslen(header); i++) if (header[i] != 'A') failures++;
            count = sizeof(header);
            result = WinHttpQueryHeaders(request, WINHTTP_QUERY_PROXY_CONNECTION, NULL, header, &count, NULL);
            if (!result || wcslen(header) != 40) failures++;
            for (unsigned i = 0; result && i < wcslen(header); i++) if (header[i] != 'B') failures++;
            printf("fixed oversized connection headers received: failures=%d\n", failures);
            fflush(stdout);
        }
        count = 0;
        if (result) result = WinHttpReadData(request, buffer, sizeof(buffer), &count);
        if (!result || count != 1 || buffer[0] != 'x') failures++;
        count = 1;
        if (result) result = WinHttpReadData(request, buffer, sizeof(buffer), &count);
        if (!result || count != 0) failures++;
        printf("connection headers: result=%d bytes=%lu failures=%d\n", result, count, failures);
        if (request) WinHttpCloseHandle(request);
        if (connection) WinHttpCloseHandle(connection);
    }
    else if (!wcscmp(argv[1], L"pac"))
    {
        swprintf(pac_url, 128, L"http://127.0.0.1:%d/proxy.pac", port);
        options.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL | WINHTTP_AUTOPROXY_HOST_LOWERCASE;
        options.lpszAutoConfigUrl = pac_url;
        result = WinHttpGetProxyForUrl(session, L"http://EXAMPLE.COM/test", &options, &proxy);
        if (!result || proxy.dwAccessType != WINHTTP_ACCESS_TYPE_NO_PROXY) failures++;
        printf("PAC normal hostname: result=%d error=%lu failures=%d\n", result, GetLastError(), failures);
        fflush(stdout);
        if (proxy.lpszProxy) GlobalFree(proxy.lpszProxy);
        if (proxy.lpszProxyBypass) GlobalFree(proxy.lpszProxyBypass);
        memset(&proxy, 0, sizeof(proxy));
        wcscpy(long_url, L"http://");
        for (int i = 7; i < 1100; i++) long_url[i] = 'A';
        wcscpy(long_url + 1100, L"/test");
        SetLastError(0);
        result = WinHttpGetProxyForUrl(session, long_url, &options, &proxy);
        error = GetLastError();
        if (result) failures++;
        printf("PAC excessive hostname: result=%d error=%lu failures=%d\n", result, error, failures);
        fflush(stdout);
        if (proxy.lpszProxy) GlobalFree(proxy.lpszProxy);
        if (proxy.lpszProxyBypass) GlobalFree(proxy.lpszProxyBypass);
    }
    else failures++;
    WinHttpCloseHandle(session);
    return failures ? 1 : 0;
}
