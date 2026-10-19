/* LGPL-2.1-or-later. Checks GL_ARB_ES2_compatibility's calls in a legacy
 * context (0130). A context made with wglCreateContext() is a legacy 2.1 context
 * on macOS, which does not list the extension. wglGetProcAddress() must still
 * answer glClearDepthf, glDepthRangef, glGetShaderPrecisionFormat,
 * glReleaseShaderCompiler and glShaderBinary; glClearDepthf and glDepthRangef must
 * take effect, and glGetShaderPrecisionFormat must answer single precision floats.
 * Then, in a 4.1 core context where present, GL_NUM_SHADER_BINARY_FORMATS is
 * reported, the case in which glShaderBinary doing nothing could matter. */
#include <windows.h>
#include <GL/gl.h>
#include <stdio.h>

#define GL_DEPTH_CLEAR_VALUE 0x0B73
#define GL_DEPTH_RANGE 0x0B70
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_HIGH_FLOAT 0x8DF2
#define GL_NUM_SHADER_BINARY_FORMATS 0x8DF9
#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

typedef void (APIENTRY *clear_depthf_fn)(GLfloat);
typedef void (APIENTRY *depth_rangef_fn)(GLfloat, GLfloat);
typedef void (APIENTRY *precision_fn)(GLenum, GLenum, GLint *, GLint *);
typedef HGLRC (WINAPI *create_attribs_fn)(HDC, HGLRC, const int *);

static int failures;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); ++failures; } } while (0)

int main(void)
{
    static const char *names[] = {"glClearDepthf", "glDepthRangef", "glGetShaderPrecisionFormat",
                                  "glReleaseShaderCompiler", "glShaderBinary"};
    PIXELFORMATDESCRIPTOR pfd = {0};
    HWND hwnd;
    HDC dc;
    HGLRC legacy, core;
    unsigned int i;
    GLfloat value, range[2];
    GLint prange[2] = {0, 0}, precision = 0, formats = -1;
    create_attribs_fn create_attribs;

    setvbuf(stdout, NULL, _IONBF, 0);
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    hwnd = CreateWindowA("static", "gl probe", WS_POPUP, 0, 0, 64, 64, NULL, NULL, NULL, NULL);
    dc = GetDC(hwnd);
    CHECK(SetPixelFormat(dc, ChoosePixelFormat(dc, &pfd), &pfd), "SetPixelFormat");
    legacy = wglCreateContext(dc);
    CHECK(legacy && wglMakeCurrent(dc, legacy), "legacy context current");
    if (!legacy) return 1;
    printf("legacy GL_VERSION=%s\n", (const char *)glGetString(GL_VERSION));

    for (i = 0; i < ARRAYSIZE(names); i++)
    {
        PROC p = wglGetProcAddress(names[i]);
        printf("%s=%s\n", names[i], p ? "found" : "missing");
        CHECK(p != NULL, names[i]);
    }
    if (!failures)
    {
        ((clear_depthf_fn)(void *)wglGetProcAddress("glClearDepthf"))(0.25f);
        glGetFloatv(GL_DEPTH_CLEAR_VALUE, &value);
        CHECK(value == 0.25f, "glClearDepthf sets the depth clear value");
        ((depth_rangef_fn)(void *)wglGetProcAddress("glDepthRangef"))(0.125f, 0.75f);
        glGetFloatv(GL_DEPTH_RANGE, range);
        CHECK(range[0] == 0.125f && range[1] == 0.75f, "glDepthRangef sets the depth range");
        ((precision_fn)(void *)wglGetProcAddress("glGetShaderPrecisionFormat"))(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT,
                                                                               prange, &precision);
        CHECK(prange[0] == 127 && prange[1] == 127 && precision == 23, "high float precision is single precision");
    }

    create_attribs = (create_attribs_fn)(void *)wglGetProcAddress("wglCreateContextAttribsARB");
    if (create_attribs)
    {
        static const int attribs[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 4, WGL_CONTEXT_MINOR_VERSION_ARB, 1,
                                      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, 0};
        if ((core = create_attribs(dc, NULL, attribs)) && wglMakeCurrent(dc, core))
        {
            glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &formats);
            printf("core GL_VERSION=%s GL_NUM_SHADER_BINARY_FORMATS=%d\n",
                   (const char *)glGetString(GL_VERSION), formats);
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(core);
        }
        else printf("no 4.1 core context\n");
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(legacy);
    ReleaseDC(hwnd, dc);
    DestroyWindow(hwnd);
    printf(failures ? "RESULT: FAIL (%d)\n" : "RESULT: PASS\n", failures);
    return failures ? 1 : 0;
}
