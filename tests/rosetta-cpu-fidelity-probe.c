/* LGPL-2.1-or-later. Checks Rosetta CPU fidelity (0157-0159) on macOS under Rosetta.
 *
 * Run it twice from a fresh prefix, as rosetta-cpu-fidelity-probe.exe:
 *   on:  with HKCU\Software\Wine\AppDefaults\rosetta-cpu-fidelity-probe.exe\Runeon
 *        DWORD RosettaCpuFidelity=1, argument "on";
 *   off: without that value, argument "off".
 * With "on" it checks that an execution breakpoint the thread sets on itself
 * fires before its instruction, with DR6 and the debug registers read back;
 * that a trap flag the program sets in its handler's context still raises a
 * single step (DR6.BS) while a breakpoint is armed;
 * that the trap flag used for stepping is hidden from pushf, the exception
 * context and GetThreadContext; that a breakpoint on a faulting instruction
 * fires before the fault and the resume flag skips it once; that reading a
 * non-canonical address reports address -1; and that CPUID leaf 7 advertises
 * CLFLUSHOPT, CLWB and RDPID, which execute. With "off" it checks that none of
 * this is in effect: the breakpoint does not fire and leaf 7 lacks the bits.
 * It does not cover the WoW64 path, breakpoints set on other threads, data
 * breakpoints (not emulated), the Rosetta table layout on other macOS
 * versions, or timing. Build: x86_64-w64-mingw32-gcc -O1 -o
 * rosetta-cpu-fidelity-probe.exe rosetta-cpu-fidelity-probe.c */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <cpuid.h>

static int failures;

#define CHECK(condition, label) do { if (!(condition)) { \
    printf("FAIL: %s (line %d)\n", label, __LINE__); ++failures; } } while (0)

/* mov (%rax),%rax; ret -- faults with rax = 0 */
__asm__( ".globl fault_insn\nfault_insn:\n\tmovq (%rax), %rax\n\tret\n" );
extern void fault_insn(void);

__attribute__((noinline)) static int target( int x ) { return x * 3 + 1; }

static volatile LONG hits, illegal, av_count, ss_count, order[8], order_n;
static volatile int rearm_after_ss, skip_len, fault_test, tf_test;
static volatile LONG tf_steps;
static volatile DWORD64 tf_dr6, tf_rip, tf_expected_rip;
static volatile DWORD64 hit_dr6, hit_rip, hit_eflags;
static volatile ULONG_PTR av_info0 = 7, av_info1 = 7;

static LONG CALLBACK handler( EXCEPTION_POINTERS *ep )
{
    EXCEPTION_RECORD *rec = ep->ExceptionRecord;
    CONTEXT *ctx = ep->ContextRecord;

    if (rec->ExceptionCode == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
        illegal++;
        ctx->Rip += skip_len;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (fault_test && ctx->Rip == (DWORD64)fault_insn)
    {
        if (order_n < 8) order[order_n++] = rec->ExceptionCode;
        if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            if (++av_count == 1)
            {
                /* arm an execution breakpoint on the faulting instruction and retry it */
                ctx->Dr0 = (DWORD64)fault_insn;
                ctx->Dr7 = 1;
                return EXCEPTION_CONTINUE_EXECUTION;
            }
            ctx->Dr7 = 0;
            ctx->Rip += 3;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (rec->ExceptionCode == EXCEPTION_SINGLE_STEP)
        {
            ss_count++;
            if (!rearm_after_ss) ctx->Dr7 = 0;
            return EXCEPTION_CONTINUE_EXECUTION;  /* same instruction again */
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (tf_test)
    {
        if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
        {
            /* skip mov (%rax),%rax and single-step the next instruction */
            ctx->Rip += 3;
            tf_expected_rip = ctx->Rip + 1;  /* after the nop */
            ctx->EFlags |= 0x100;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        if (rec->ExceptionCode == EXCEPTION_SINGLE_STEP)
        {
            tf_steps++;
            tf_dr6 = ctx->Dr6;
            tf_rip = ctx->Rip;
            ctx->Dr7 = 0;
            return EXCEPTION_CONTINUE_EXECUTION;
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (rec->ExceptionCode == EXCEPTION_SINGLE_STEP)
    {
        hits++;
        hit_dr6 = ctx->Dr6;
        hit_rip = ctx->Rip;
        hit_eflags = ctx->EFlags;
        ctx->Dr7 = 0;
        ctx->Dr6 = 0;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION)
    {
        av_info0 = rec->ExceptionInformation[0];
        av_info1 = rec->ExceptionInformation[1];
        ctx->Rip += 3;  /* mov (%rax),%rax */
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void run_fault( int rearm )
{
    av_count = ss_count = order_n = 0;
    rearm_after_ss = rearm;
    fault_test = 1;
    __asm__ volatile ( "xorl %%eax, %%eax\n\tcall fault_insn" ::: "rax", "memory" );
    fault_test = 0;
}

int main( int argc, char **argv )
{
    static const ULONGLONG noncanonical = 0xfe9d7ac749292694ull;
    static volatile char line[64];
    CONTEXT ctx;
    unsigned int a, b, c, d;
    unsigned long long flags;
    ULONG_PTR id;
    DWORD64 ctx_eflags, read_dr0, read_dr7;
    int on, result, has_bits;
    LONG before;

    if (argc != 2 || (strcmp( argv[1], "on" ) && strcmp( argv[1], "off" )))
    {
        printf( "usage: rosetta-cpu-fidelity-probe.exe on|off\n" );
        return 2;
    }
    on = !strcmp( argv[1], "on" );
    AddVectoredExceptionHandler( 1, handler );

    /* execution breakpoint on target(), set on this thread */
    memset( &ctx, 0, sizeof(ctx) );
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ctx.Dr0 = (DWORD64)target;
    ctx.Dr7 = 1;  /* L0, execute, length 1 */
    CHECK( SetThreadContext( GetCurrentThread(), &ctx ), "SetThreadContext" );
    /* nothing that prints until the breakpoint is disarmed: every instruction is stepped */
    __asm__ volatile ( "pushfq\n\tpopq %0" : "=r"(flags) );
    memset( &ctx, 0, sizeof(ctx) );
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS | CONTEXT_CONTROL;
    GetThreadContext( GetCurrentThread(), &ctx );
    read_dr0 = ctx.Dr0;
    read_dr7 = ctx.Dr7;
    ctx_eflags = ctx.EFlags;
    result = target( 13 );
    result += target( 14 );
    printf( "breakpoint: hits=%ld rip_ok=%d dr6=%#llx pushf_tf=%d context_tf=%d getcontext_tf=%d dr0_ok=%d dr7=%#llx\n",
            hits, hit_rip == (DWORD64)target, (unsigned long long)hit_dr6, (int)((flags >> 8) & 1),
            (int)((hit_eflags >> 8) & 1), (int)((ctx_eflags >> 8) & 1), read_dr0 == (DWORD64)target,
            (unsigned long long)read_dr7 );
    CHECK( result == 40 + 43, "target() results" );
    if (on)
    {
        CHECK( hits == 1, "breakpoint fires once, then is disarmed by the handler" );
        CHECK( hit_rip == (DWORD64)target, "breakpoint reported at its address" );
        CHECK( (hit_dr6 & 0xf) == 1, "DR6 names DR0" );
        CHECK( read_dr0 == (DWORD64)target && (read_dr7 & 3) == 1, "debug registers read back" );
        CHECK( !((flags >> 8) & 1), "pushf does not show the trap flag" );
        CHECK( !((hit_eflags >> 8) & 1), "exception context does not show the trap flag" );
        CHECK( !((ctx_eflags >> 8) & 1), "GetThreadContext does not show the trap flag" );
    }
    else CHECK( hits == 0, "breakpoint does not fire without the opt-in" );

    /* the program's own trap flag while a breakpoint (on an address never reached) is armed */
    memset( &ctx, 0, sizeof(ctx) );
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    ctx.Dr0 = (DWORD64)fault_insn;
    ctx.Dr7 = 1;
    SetThreadContext( GetCurrentThread(), &ctx );
    tf_test = 1;
    __asm__ volatile ( "xorl %%eax, %%eax\n\tmovq (%%rax), %%rax\n\tnop" ::: "rax", "memory" );
    tf_test = 0;
    memset( &ctx, 0, sizeof(ctx) );
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    SetThreadContext( GetCurrentThread(), &ctx );
    printf( "program trap flag: steps=%ld rip_ok=%d dr6=%#llx\n", tf_steps, tf_rip == tf_expected_rip,
            (unsigned long long)tf_dr6 );
    CHECK( tf_steps == 1 && tf_rip == tf_expected_rip, "program's trap flag raises one single step after the next instruction" );
    if (on) CHECK( (tf_dr6 & 0x400f) == 0x4000, "DR6 reports BS only" );

    /* breakpoint on a faulting instruction */
    run_fault( 0 );
    printf( "fault+breakpoint: av=%ld ss=%ld order=%lx,%lx,%lx\n", av_count, ss_count, order[0], order[1], order[2] );
    if (on)
    {
        CHECK( order_n == 3 && order[0] == (LONG)EXCEPTION_ACCESS_VIOLATION &&
               order[1] == (LONG)EXCEPTION_SINGLE_STEP && order[2] == (LONG)EXCEPTION_ACCESS_VIOLATION,
               "breakpoint fires before the faulting instruction" );
        run_fault( 1 );
        printf( "fault+breakpoint kept armed: av=%ld ss=%ld\n", av_count, ss_count );
        CHECK( ss_count == 1 && av_count == 2, "resume flag skips the armed breakpoint once" );
    }
    else CHECK( ss_count == 0 && av_count == 2, "no breakpoint without the opt-in" );

    /* non-canonical address */
    __asm__ volatile ( "movq %0, %%rax\n\tmovq (%%rax), %%rax" :: "r"(noncanonical) : "rax", "memory" );
    printf( "non-canonical read: info0=%llu info1=%#llx\n", (unsigned long long)av_info0, (unsigned long long)av_info1 );
    if (on) CHECK( av_info0 == 0 && av_info1 == (ULONG_PTR)-1, "non-canonical read reported as #GP" );
    else CHECK( av_info1 != (ULONG_PTR)-1, "non-canonical read unchanged without the opt-in" );

    /* CPUID leaf 7 and the instructions it advertises */
    __cpuid_count( 7, 0, a, b, c, d );
    has_bits = ((b >> 23) & 1) && ((b >> 24) & 1) && ((c >> 22) & 1);
    printf( "leaf7: eax=%08x ebx=%08x ecx=%08x edx=%08x\n", a, b, c, d );
    if (!on)
    {
        CHECK( !((b >> 24) & 1) && !((c >> 22) & 1), "leaf 7 lacks CLWB and RDPID without the opt-in" );
        printf( failures ? "RESULT FAIL (%d)\n" : "RESULT PASS\n", failures );
        return failures != 0;
    }
    CHECK( has_bits, "leaf 7 advertises CLFLUSHOPT, CLWB and RDPID" );

    before = illegal;
    skip_len = 4;
    __asm__ volatile ( ".byte 0x66, 0x0f, 0xae, 0x38" :: "a"(line) : "memory" );  /* clflushopt (%rax) */
    CHECK( illegal == before, "clflushopt executes" );
    __asm__ volatile ( ".byte 0x66, 0x0f, 0xae, 0x30" :: "a"(line) : "memory" );  /* clwb (%rax) */
    CHECK( illegal == before, "clwb executes" );
    skip_len = 6;
    __asm__ volatile ( "movq %0, %%r8\n\t.byte 0x66, 0x41, 0x0f, 0xae, 0x70, 0x08" :: "r"(line) : "memory", "r8" );  /* clwb 8(%r8) */
    CHECK( illegal == before, "clwb with REX and disp8 executes" );

    skip_len = 4;
    id = 0xdead;
    __asm__ volatile ( ".byte 0xf3, 0x0f, 0xc7, 0xf8" : "=a"(id) );  /* rdpid %rax */
    printf( "rdpid: rax=%llu GetCurrentProcessorNumber=%lu\n", (unsigned long long)id, GetCurrentProcessorNumber() );
    CHECK( illegal == before && id < 256, "rdpid %rax returns a processor number" );
    skip_len = 5;
    __asm__ volatile ( "movq $0xdead, %%r9\n\t.byte 0xf3, 0x41, 0x0f, 0xc7, 0xf9\n\tmovq %%r9, %0" : "=r"(id) :: "r9" );  /* rdpid %r9 */
    CHECK( illegal == before && id < 256, "rdpid %r9 returns a processor number" );

    printf( failures ? "RESULT FAIL (%d)\n" : "RESULT PASS\n", failures );
    return failures != 0;
}
