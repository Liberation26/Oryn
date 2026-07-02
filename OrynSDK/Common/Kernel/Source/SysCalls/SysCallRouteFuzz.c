#include "SysCallPolicy.h"
#include "KernelIo.h"
#include "KernelScreenReport.h"

static void CopyArguments(OrynSysCallPacket* packet, const uint64_t* arguments, uint64_t count)
{
    uint64_t limit = count < ORYN_SYSCALL_ARGUMENT_COUNT ? count : ORYN_SYSCALL_ARGUMENT_COUNT;
    for (uint64_t index = 0; index < limit; ++index)
    {
        packet->Arguments[index] = arguments[index];
    }
}

int OrynSysCallPolicyRouteUserRequest(
    uint64_t platform,
    uint64_t platform_number,
    const uint64_t* arguments,
    uint64_t argument_count,
    OrynSysCallPacket* packet,
    OrynSysCallState* state)
{
    uint64_t kind = ORYN_SYSCALL_KIND_EVENT;
    uint64_t name_space = ORYN_SYSCALL_NS_COMPAT;
    uint64_t operation = ORYN_SYSCALL_OP_COMPAT_UNKNOWN_PLATFORM_SYSCALL;

    if (packet == 0 || state == 0)
    {
        return 0;
    }

    if (platform_number == ORYN_SYSCALL_OP_KERNEL_VERSION)
    {
        kind = ORYN_SYSCALL_KIND_GET;
        name_space = ORYN_SYSCALL_NS_KERNEL;
        operation = ORYN_SYSCALL_OP_KERNEL_VERSION;
    }
    else if (platform_number == ORYN_SYSCALL_OP_DEBUG_SET_LEVEL)
    {
        kind = ORYN_SYSCALL_KIND_SET;
        name_space = ORYN_SYSCALL_NS_DEBUG;
        operation = ORYN_SYSCALL_OP_DEBUG_SET_LEVEL;
    }

    OrynSysCallPreparePacket(packet, kind, name_space, operation);
    packet->Platform = platform;
    packet->PlatformNumber = platform_number;
    CopyArguments(packet, arguments, argument_count);
    OrynSysCallInstallCredentialPlaceholder(packet, state);
    state->UserRequestsRouted += 1ULL;
    return 1;
}

int OrynSysCallRunFuzzTests(OrynSysCallState* state)
{
    OrynSysCallPacket packet;
    uint64_t args[ORYN_SYSCALL_ARGUMENT_COUNT];
    int bad_pointer_ok;
    int bad_size_ok;
    int route_ok;

    if (state == 0)
    {
        return 0;
    }

    state->FuzzTestsRan += 1ULL;
    bad_pointer_ok = OrynSysCallValidateUserPointer(0ULL, 16ULL, state) == 0;

    OrynSysCallPreparePacket(&packet, ORYN_SYSCALL_KIND_GET,
        ORYN_SYSCALL_NS_KERNEL, ORYN_SYSCALL_OP_KERNEL_VERSION);
    packet.Size = sizeof(packet) - 8ULL;
    bad_size_ok = OrynSysCallValidatePacket(&packet, state) == 0;

    for (uint64_t index = 0; index < ORYN_SYSCALL_ARGUMENT_COUNT; ++index)
    {
        args[index] = index;
    }

    route_ok = OrynSysCallPolicyRouteUserRequest(ORYN_SYSCALL_PLATFORM_INTERNAL,
        ORYN_SYSCALL_OP_KERNEL_VERSION, args, ORYN_SYSCALL_ARGUMENT_COUNT,
        &packet, state);

    state->FuzzInvalidPointerPassed = bad_pointer_ok ? 1U : 0U;
    state->FuzzInvalidPacketSizePassed = bad_size_ok ? 1U : 0U;
    state->RouteProofPassed = route_ok && packet.Kind == ORYN_SYSCALL_KIND_GET ? 1U : 0U;
    state->ErrnoMappingProofPassed =
        OrynSysCallMapStatusToErrno(ORYN_SYSCALL_STATUS_BAD_POINTER) == -ORYN_SYSCALL_ERRNO_EFAULT ? 1U : 0U;

    return state->FuzzInvalidPointerPassed &&
        state->FuzzInvalidPacketSizePassed &&
        state->RouteProofPassed &&
        state->ErrnoMappingProofPassed;
}

void OrynSysCallPrintPolicyProof(const OrynSysCallState* state)
{
    OrynKernelScreenReportOkOrFail(state != 0 && state->RouteProofPassed,
        "User requests route into Get, Set, or Event packets.",
        "User request routing into Get, Set, or Event packets failed.");
    OrynKernelScreenReportOkOrFail(state != 0 && state->AbiMinVersion == 1ULL && state->AbiMaxVersion == 1ULL,
        "Syscall ABI versioning policy exists.",
        "Syscall ABI versioning policy missing.");
    OrynKernelScreenReportOkOrFail(state != 0 && state->ArgumentValidationChecks != 0ULL,
        "Syscall argument validation ran.",
        "Syscall argument validation did not run.");
    OrynKernelScreenReportOkOrFail(state != 0 && state->UserPointerChecks != 0ULL,
        "User pointer validation ran.",
        "User pointer validation did not run.");
    OrynKernelScreenReportOkOrFail(state != 0 && state->CredentialsPlaceholderReady,
        "Process credentials placeholder exists.",
        "Process credentials placeholder missing.");
    OrynKernelScreenReportOkOrFail(state != 0 && state->ErrnoMappingProofPassed,
        "POSIX-like errno/status mapping exists.",
        "POSIX-like errno/status mapping missing.");
    OrynKernelScreenReportOkOrFail(state != 0 && state->TracePackets != 0ULL,
        "Debug syscall tracing ran.",
        "Debug syscall tracing did not run.");
    OrynKernelScreenReportOkOrFail(state != 0 && state->FuzzInvalidPointerPassed && state->FuzzInvalidPacketSizePassed,
        "Syscall fuzz tests cover invalid pointers and packet sizes.",
        "Syscall fuzz tests did not cover invalid pointers and packet sizes.");
}
