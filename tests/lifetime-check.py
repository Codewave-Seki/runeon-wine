#!/usr/bin/env python3
"""Compile real Wine functions against deterministic ownership/failure stubs.

This checks function-level failure-path contracts, not Wine, COM or a GPU runtime.
Temporary C source and executable are removed when the check finishes.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


def extract_function(path, name):
    text = path.read_text()
    match = re.search(r"(?m)^(?:static )?(?:void|HRESULT WINAPI) " + re.escape(name) + r"\(", text)
    if not match:
        raise ValueError(f"Function not found: {name} in {path}")
    start = text.index("{", match.start())
    depth = 0
    # Ignore braces inside C strings and comments while finding the function end.
    tokens = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*|[{}]'
    for token in re.finditer(tokens, text[start:], re.S):
        if token.group() == "{":
            depth += 1
        elif token.group() == "}":
            depth -= 1
            if not depth:
                return text[match.start():start + token.end()]
    raise ValueError(f"Unterminated function: {name}")


STUBS = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef int32_t HRESULT;
#define S_OK ((HRESULT)0)
#define E_OUTOFMEMORY ((HRESULT)0x8007000e)
#define E_INVALIDARG ((HRESULT)0x80070057)
#define FAILED(hr) ((HRESULT)(hr) < 0)
#define WINAPI
#define TRACE(...) ((void)0)
static int checks, failures, invalid_releases;
static void check(int condition, const char *name)
{
    ++checks;
    if (!condition) { ++failures; printf("{\"failed\":\"%s\"}\n", name); }
}
struct object { int refs, releases; };
static void release(struct object *object)
{
    ++object->releases;
    if (object->refs <= 0) ++invalid_releases;
    else --object->refs;
}

typedef struct object ID2D1Brush;
typedef struct object ID2D1Geometry;
struct d2d_device_context { int unused; };
struct d2d_command_fill_geometry
{
    struct { int op; } c;
    ID2D1Geometry *geometry;
    ID2D1Brush *brush, *opacity_brush;
};
struct d2d_command_list
{
    int state, creates, fail_at, owned_count, recorded;
    ID2D1Brush brushes[2];
    struct object *owned[3];
    struct d2d_command_fill_geometry command;
};
#define D2D_COMMAND_LIST_STATE_ERROR 1
#define D2D_COMMAND_FILL_GEOMETRY 9
static HRESULT d2d_command_list_create_brush(struct d2d_command_list *list,
        const struct d2d_device_context *context, ID2D1Brush *original, ID2D1Brush **out)
{
    (void)context; (void)original;
    if (++list->creates == list->fail_at) { *out = NULL; return E_OUTOFMEMORY; }
    *out = &list->brushes[list->creates - 1];
    /* The real helper returns a borrowed pointer; its sole reference is list-owned. */
    (*out)->refs = 1;
    list->owned[list->owned_count++] = *out;
    return S_OK;
}
static void ID2D1Brush_Release(ID2D1Brush *brush) { release(brush); }
static void d2d_command_list_reference_object(struct d2d_command_list *list, void *object)
{
    struct object *value = object;
    ++value->refs;
    list->owned[list->owned_count++] = value;
}
static void *d2d_command_list_require_space(struct d2d_command_list *list, size_t size)
{
    (void)size;
    ++list->recorded;
    return &list->command;
}

typedef void *HWND;
struct video_presenter
{
    int cs, creates, fail_create;
    HWND video_window;
    struct object *swapchain;
    struct object chains[3];
};
typedef struct video_presenter IMFVideoDisplayControl;
static struct video_presenter *impl_from_IMFVideoDisplayControl(IMFVideoDisplayControl *iface) { return iface; }
static int IsWindow(HWND window) { return (uintptr_t)window >= 1 && (uintptr_t)window <= 3; }
static void EnterCriticalSection(int *depth) { ++*depth; }
static void LeaveCriticalSection(int *depth) { --*depth; }
static void IDirect3DSwapChain9_Release(struct object *chain) { release(chain); }
static HRESULT video_presenter_create_swapchain(struct video_presenter *presenter)
{
    ++presenter->creates;
    /* A failed video_presenter_get_device returns without writing swapchain. */
    if (presenter->fail_create) return E_OUTOFMEMORY;
    presenter->swapchain = &presenter->chains[presenter->creates - 1];
    presenter->swapchain->refs = 1;
    return S_OK;
}
'''

TESTS = r'''
static void test_d2d(int fail_at, int opacity)
{
    struct d2d_command_list list = {0};
    struct d2d_device_context context = {0};
    ID2D1Geometry geometry = {1, 0};
    ID2D1Brush original = {1, 0}, original_opacity = {1, 0};
    int i, created, bad_before = invalid_releases;
    list.fail_at = fail_at;
    d2d_command_list_fill_geometry(&list, &context, &geometry, &original,
            opacity ? &original_opacity : NULL);
    created = fail_at ? fail_at - 1 : 1 + opacity;
    check(list.state == (fail_at ? D2D_COMMAND_LIST_STATE_ERROR : 0), "d2d error state");
    check(list.creates == (fail_at ? fail_at : 1 + opacity), "d2d dependency call count");
    check(list.recorded == !fail_at, "d2d failed command must not be recorded");
    check(original.refs == 1 && !original.releases && original_opacity.refs == 1 && !original_opacity.releases,
            "d2d caller retains original brushes");
    for (i = 0; i < created; ++i)
        check(list.brushes[i].refs == 1 && !list.brushes[i].releases,
                "d2d list-owned brush must remain alive until list destruction");
    if (!fail_at)
        check(list.command.geometry == &geometry && list.command.brush == &list.brushes[0]
                && list.command.opacity_brush == (opacity ? &list.brushes[1] : NULL)
                && list.command.c.op == D2D_COMMAND_FILL_GEOMETRY, "d2d successful command retains intended objects");
    for (i = 0; i < list.owned_count; ++i) release(list.owned[i]);
    for (i = 0; i < created; ++i)
        check(list.brushes[i].refs == 0 && list.brushes[i].releases == 1,
                "d2d each recorded brush released exactly once by list destruction");
    check(geometry.refs == 1, "d2d geometry caller ownership preserved");
    check(invalid_releases == bad_before, "d2d no double release during destruction");
}

static void test_evr(void)
{
    struct video_presenter p = {0}, first_failure = {0}, success = {0};
    HRESULT hr;
    int bad_before = invalid_releases;
    hr = video_presenter_control_SetVideoWindow(&p, NULL);
    check(hr == E_INVALIDARG && !p.creates && !p.swapchain && !p.cs, "evr invalid window preserves state");
    hr = video_presenter_control_SetVideoWindow(&p, (HWND)1);
    check(hr == S_OK && p.swapchain == &p.chains[0] && p.chains[0].refs == 1, "evr initial creation owns one reference");
    hr = video_presenter_control_SetVideoWindow(&p, (HWND)1);
    check(hr == S_OK && p.creates == 1 && !p.chains[0].releases, "evr same window retains existing chain");
    p.fail_create = 1;
    hr = video_presenter_control_SetVideoWindow(&p, (HWND)2);
    check(hr == E_OUTOFMEMORY && p.creates == 2, "evr rebuild propagates failure");
    check(!p.swapchain, "evr failed rebuild must not retain a released swapchain");
    check(p.chains[0].refs == 0 && p.chains[0].releases == 1, "evr failed rebuild releases old chain once");
    check(!p.cs, "evr failed rebuild balances lock");
    /* Continue after a failed rebuild to detect stale-pointer release predictably. */
    p.fail_create = 0;
    hr = video_presenter_control_SetVideoWindow(&p, (HWND)3);
    check(hr == S_OK && p.swapchain == &p.chains[2], "evr later window can create a new chain");
    check(p.chains[0].releases == 1, "evr later rebuild must not release old chain again");
    if (p.swapchain) release(p.swapchain);
    check(p.chains[2].refs == 0 && p.chains[2].releases == 1, "evr replacement ownership released once at destruction");
    check(invalid_releases == bad_before, "evr no stale swapchain release");

    first_failure.fail_create = 1;
    hr = video_presenter_control_SetVideoWindow(&first_failure, (HWND)1);
    check(hr == E_OUTOFMEMORY && !first_failure.swapchain && !first_failure.cs,
            "evr first creation failure owns no chain and preserves error");
    first_failure.fail_create = 0;
    hr = video_presenter_control_SetVideoWindow(&first_failure, (HWND)2);
    check(hr == S_OK && first_failure.swapchain == &first_failure.chains[1], "evr creation works after initial failure");
    if (first_failure.swapchain) release(first_failure.swapchain);
    check(first_failure.chains[1].refs == 0 && first_failure.chains[1].releases == 1,
            "evr initial failure recovery has no leaked ownership");

    hr = video_presenter_control_SetVideoWindow(&success, (HWND)1);
    check(hr == S_OK, "evr success case initial creation");
    hr = video_presenter_control_SetVideoWindow(&success, (HWND)2);
    check(hr == S_OK && success.swapchain == &success.chains[1]
            && success.chains[0].refs == 0 && success.chains[0].releases == 1
            && success.chains[1].refs == 1 && !success.cs, "evr successful replacement transfers ownership");
    if (success.swapchain) release(success.swapchain);
    check(success.chains[1].releases == 1 && success.chains[1].refs == 0, "evr successful replacement final release");
}
int main(void)
{
    test_d2d(1, 1);
    test_d2d(2, 1);
    test_d2d(0, 1);
    test_d2d(0, 0);
    test_evr();
    printf("{\"checks\":%d,\"failures\":%d,\"invalid_releases\":%d}\n", checks, failures, invalid_releases);
    return failures ? 1 : 0;
}
'''


def main():
    if len(sys.argv) != 2:
        raise SystemExit(f"usage: {sys.argv[0]} SOURCE_ROOT")
    root = Path(sys.argv[1]).resolve(strict=True)
    functions = {}
    for relative, name in [("dlls/d2d1/command_list.c", "d2d_command_list_fill_geometry"),
                           ("dlls/evr/presenter.c", "video_presenter_control_SetVideoWindow")]:
        functions[name] = extract_function(root / relative, name)
    with tempfile.TemporaryDirectory(prefix="runeon-lifetime-check-") as temporary:
        c_path = Path(temporary) / "check.c"
        binary = Path(temporary) / "check"
        c_path.write_text(STUBS + "\n" + "\n\n".join(functions.values()) + TESTS)
        build = subprocess.run(["clang", "-std=c11", "-Wall", "-Wextra", "-Werror",
                                "-Wno-unused-function", "-O0", str(c_path), "-o", str(binary)],
                               capture_output=True, text=True, timeout=60)
        if build.returncode:
            print(build.stdout + build.stderr, file=sys.stderr)
            return 2
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
        observations = [json.loads(line) for line in result.stdout.splitlines()]
        print(json.dumps({"sourceRoot": str(root),
                          "functionSha256": {name: hashlib.sha256(code.encode()).hexdigest()
                                             for name, code in functions.items()},
                          "validation": "real source functions with dependency failure injection; no Wine or GPU execution",
                          "build": "clang -std=c11 -Wall -Wextra -Werror -Wno-unused-function -O0",
                          "observations": observations,
                          "stderr": result.stderr, "exitCode": result.returncode}, indent=2))
        return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
