#include "OrynBuild.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

int OrynRunCommand(const char* command)
{
    printf("[CMD ] %s\n", command);
    int code = system(command);
    if (code == -1)
    {
        return 0;
    }
    if (WIFEXITED(code) && WEXITSTATUS(code) == 0)
    {
        return 1;
    }
    return 0;
}

int OrynRunCommandCapture(const char* command, char* output, size_t output_size)
{
    output[0] = 0;
    FILE* pipe = popen(command, "r");
    if (pipe == 0)
    {
        return 0;
    }

    size_t used = 0;
    while (used + 1 < output_size)
    {
        size_t read_count = fread(output + used, 1, output_size - used - 1, pipe);
        used += read_count;
        if (read_count == 0)
        {
            break;
        }
    }
    output[used] = 0;

    int code = pclose(pipe);
    return WIFEXITED(code) && WEXITSTATUS(code) == 0;
}

void OrynShellQuote(char* output, size_t output_size, const char* input)
{
    size_t used = 0;
    if (output_size == 0)
    {
        return;
    }

    output[used++] = '\'';
    for (const char* current = input; *current != 0 && used + 5 < output_size; ++current)
    {
        if (*current == '\'')
        {
            output[used++] = '\'';
            output[used++] = '\\';
            output[used++] = '\'';
            output[used++] = '\'';
        }
        else
        {
            output[used++] = *current;
        }
    }
    if (used + 1 < output_size)
    {
        output[used++] = '\'';
    }
    output[used] = 0;
}

static void StripFirstLine(char* text)
{
    char* newline = strchr(text, '\n');
    if (newline != 0)
    {
        *newline = 0;
    }
}

int OrynFindProgram(const char* program, char* output, size_t output_size)
{
    char command[512];
    snprintf(command, sizeof(command), "command -v %s 2>/dev/null", program);
    if (!OrynRunCommandCapture(command, output, output_size))
    {
        output[0] = 0;
        return 0;
    }

    StripFirstLine(output);
    return output[0] != 0;
}

int OrynConvertWslPathToWindows(const char* wsl_path, char* output, size_t output_size)
{
    char quoted[ORYN_MAX_PATH + 16];
    char command[ORYN_MAX_PATH + 64];
    OrynShellQuote(quoted, sizeof(quoted), wsl_path);
    snprintf(command, sizeof(command), "wslpath -w %s 2>/dev/null", quoted);

    if (!OrynRunCommandCapture(command, output, output_size))
    {
        output[0] = 0;
        return 0;
    }

    StripFirstLine(output);
    return output[0] != 0;
}

int OrynConvertWslPathToWindowsQemu(const char* wsl_path, char* output, size_t output_size)
{
    char quoted[ORYN_MAX_PATH + 16];
    char command[ORYN_MAX_PATH + 64];
    OrynShellQuote(quoted, sizeof(quoted), wsl_path);
    snprintf(command, sizeof(command), "wslpath -m %s 2>/dev/null", quoted);

    if (!OrynRunCommandCapture(command, output, output_size))
    {
        output[0] = 0;
        return 0;
    }

    StripFirstLine(output);
    return output[0] != 0;
}

int OrynConvertWindowsPathToWsl(const char* windows_path, char* output, size_t output_size)
{
    char quoted[ORYN_MAX_PATH + 16];
    char command[ORYN_MAX_PATH + 64];
    OrynShellQuote(quoted, sizeof(quoted), windows_path);
    snprintf(command, sizeof(command), "wslpath -u %s 2>/dev/null", quoted);

    if (!OrynRunCommandCapture(command, output, output_size))
    {
        output[0] = 0;
        return 0;
    }

    StripFirstLine(output);
    return output[0] != 0;
}

static int AcceptQemuCandidate(const char* candidate, char* output, size_t output_size)
{
    if (candidate == 0 || candidate[0] == 0)
    {
        return 0;
    }

    char wsl_candidate[ORYN_MAX_PATH];
    if (strchr(candidate, ':') != 0 && strstr(candidate, "/mnt/") != candidate)
    {
        if (!OrynConvertWindowsPathToWsl(candidate, wsl_candidate, sizeof(wsl_candidate)))
        {
            return 0;
        }
    }
    else
    {
        snprintf(wsl_candidate, sizeof(wsl_candidate), "%s", candidate);
    }

    if (!OrynFileExists(wsl_candidate))
    {
        return 0;
    }

    snprintf(output, output_size, "%s", wsl_candidate);
    return 1;
}

static int AcceptOvmfCandidate(const char* candidate, char* output, size_t output_size)
{
    if (candidate == 0 || candidate[0] == 0)
    {
        return 0;
    }

    char wsl_candidate[ORYN_MAX_PATH];
    if (strchr(candidate, ':') != 0 && strstr(candidate, "/mnt/") != candidate)
    {
        if (!OrynConvertWindowsPathToWsl(candidate, wsl_candidate, sizeof(wsl_candidate)))
        {
            return 0;
        }
    }
    else
    {
        snprintf(wsl_candidate, sizeof(wsl_candidate), "%s", candidate);
    }

    if (!OrynFileExists(wsl_candidate))
    {
        return 0;
    }

    snprintf(output, output_size, "%s", wsl_candidate);
    return 1;
}

int OrynFindWindowsQemu(char* output, size_t output_size)
{
    const char* env_candidates[] =
    {
        getenv("ORYN_QEMU_WINDOWS_PATH"),
        getenv("ORYN_QEMU_EXE"),
        getenv("ORYN_QEMU_PATH"),
        0
    };

    for (int index = 0; env_candidates[index] != 0; ++index)
    {
        if (AcceptQemuCandidate(env_candidates[index], output, output_size))
        {
            return 1;
        }
    }

    char found[ORYN_MAX_PATH];
    if (OrynFindProgram("qemu-system-x86_64.exe", found, sizeof(found)))
    {
        if (AcceptQemuCandidate(found, output, output_size))
        {
            return 1;
        }
    }

    const char* candidates[] =
    {
        "/mnt/c/Program Files/qemu/qemu-system-x86_64.exe",
        "/mnt/c/Program Files/QEMU/qemu-system-x86_64.exe",
        "/mnt/c/Oryn/Tools/QEMU/qemu-system-x86_64.exe",
        "/mnt/c/msys64/ucrt64/bin/qemu-system-x86_64.exe",
        "/mnt/c/msys64/mingw64/bin/qemu-system-x86_64.exe",
        0
    };

    for (int index = 0; candidates[index] != 0; ++index)
    {
        if (AcceptQemuCandidate(candidates[index], output, output_size))
        {
            return 1;
        }
    }

    output[0] = 0;
    return 0;
}

int OrynFindOvmf(char* output, size_t output_size)
{
    const char* env_candidates[] =
    {
        getenv("ORYN_OVMF_PATH"),
        getenv("ORYN_OVMF_WINDOWS_PATH"),
        getenv("ORYN_OVMF_CODE"),
        0
    };

    for (int index = 0; env_candidates[index] != 0; ++index)
    {
        if (AcceptOvmfCandidate(env_candidates[index], output, output_size))
        {
            return 1;
        }
    }

    const char* candidates[] =
    {
        "/mnt/c/Program Files/qemu/share/edk2-x86_64-code.fd",
        "/mnt/c/Program Files/qemu/share/OVMF_CODE.fd",
        "/mnt/c/Program Files/qemu/share/ovmf-x86_64-code.fd",
        "/mnt/c/Program Files/QEMU/share/edk2-x86_64-code.fd",
        "/mnt/c/Program Files/QEMU/share/OVMF_CODE.fd",
        "/mnt/c/Oryn/Tools/OVMF/OVMF_CODE.fd",
        "/mnt/c/Users/daves/AppData/Local/Temp/OrynRepoUpdate-c7e51e95a6fa487c982cf07ba2d1e2cb/Tools/OVMF/OVMF_CODE.fd",
        "/usr/share/OVMF/OVMF_CODE.fd",
        "/usr/share/ovmf/OVMF.fd",
        "/usr/share/qemu/OVMF_CODE.fd",
        0
    };

    for (int index = 0; candidates[index] != 0; ++index)
    {
        if (AcceptOvmfCandidate(candidates[index], output, output_size))
        {
            return 1;
        }
    }

    output[0] = 0;
    return 0;
}
