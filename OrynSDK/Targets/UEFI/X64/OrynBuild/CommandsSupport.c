#include "CommandsSupport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

void PrintToolStatus(const char* label, const char* program, int* ok)
{
    char path[ORYN_MAX_PATH];
    if (OrynFindProgram(program, path, sizeof(path)))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "%s found: %s", label, path);
        OrynLogOk(message);
    }
    else
    {
        char message[256];
        snprintf(message, sizeof(message), "%s was not found: %s", label, program);
        OrynLogFail(message);
        *ok = 0;
    }
}

void PrintWindowsQemuStatus(int* ok)
{
    char qemu[ORYN_MAX_PATH];
    if (OrynFindWindowsQemu(qemu, sizeof(qemu)))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "Windows QEMU found: %s", qemu);
        OrynLogOk(message);
        return;
    }

    OrynLogFail("Windows QEMU was not found from WSL.");
    OrynLogWarn("Expected default Windows QEMU path: /mnt/c/Program Files/qemu/qemu-system-x86_64.exe");
    OrynLogWarn("Optional override: ORYN_QEMU_WINDOWS_PATH");
    *ok = 0;
}

void PrintFileIfPresent(const char* title, const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == 0)
    {
        return;
    }

    char file_message[ORYN_MAX_PATH + 128];
    snprintf(file_message, sizeof(file_message), "%s: %s", title, path);
    OrynLogInfo(file_message);
    printf("-------- %s --------\n", title);

    int value;
    int wrote_any = 0;
    while ((value = fgetc(file)) != EOF)
    {
        putchar(value);
        wrote_any = 1;
    }

    if (!wrote_any)
    {
        OrynLogInfo("[empty]");
    }
    else
    {
        printf("\n");
    }

    printf("------ end %s ------\n", title);
    fclose(file);
}

int ReadFileText(const char* path, char* output, size_t output_size)
{
    output[0] = 0;
    FILE* file = fopen(path, "rb");
    if (file == 0)
    {
        return 0;
    }

    size_t used = fread(output, 1, output_size - 1, file);
    output[used] = 0;
    fclose(file);
    return 1;
}

int TextContains(const char* text, const char* needle)
{
    return strstr(text, needle) != 0;
}

const char* PassFail(int value)
{
    return value ? "PASS" : "FAIL";
}

static int TextEqualsIgnoreCase(const char* left, const char* right)
{
    if (left == 0 || right == 0)
    {
        return 0;
    }

    while (*left != 0 && *right != 0)
    {
        char a = *left;
        char b = *right;
        if (a >= 'A' && a <= 'Z')
        {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z')
        {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b)
        {
            return 0;
        }
        ++left;
        ++right;
    }

    return *left == 0 && *right == 0;
}

const char* ResolveQemuDisplayMode(const OrynProject* project)
{
    if (project->run_display[0] == 0)
    {
        return "none";
    }

    if (TextEqualsIgnoreCase(project->run_display, "headless") ||
        TextEqualsIgnoreCase(project->run_display, "yes") ||
        TextEqualsIgnoreCase(project->run_display, "true"))
    {
        return "none";
    }

    if (TextEqualsIgnoreCase(project->run_display, "window") ||
        TextEqualsIgnoreCase(project->run_display, "windowed") ||
        TextEqualsIgnoreCase(project->run_display, "no") ||
        TextEqualsIgnoreCase(project->run_display, "false"))
    {
        return "sdl";
    }

    return project->run_display;
}

int IsSafeQemuDisplayMode(const char* display)
{
    if (display == 0 || display[0] == 0)
    {
        return 0;
    }

    for (int index = 0; display[index] != 0; ++index)
    {
        char ch = display[index];
        if (!((ch >= 'A' && ch <= 'Z') ||
              (ch >= 'a' && ch <= 'z') ||
              (ch >= '0' && ch <= '9') ||
              ch == '_' || ch == '-' || ch == ',' || ch == '=' || ch == ':'))
        {
            return 0;
        }
    }

    return 1;
}

int RunQemuAndGetExitCode(const char* command, int* exit_code)
{
    *exit_code = -1;
    OrynLogCommand(command);

    int code = system(command);
    if (code == -1)
    {
        return 0;
    }

    if (!WIFEXITED(code))
    {
        return 0;
    }

    *exit_code = WEXITSTATUS(code);
    if (*exit_code == 0 || *exit_code == 33 || *exit_code == 124 || *exit_code == 137)
    {
        return 1;
    }

    char qemu_warn[128];
    snprintf(qemu_warn, sizeof(qemu_warn), "QEMU exited with code %d.", *exit_code);
    OrynLogWarn(qemu_warn);

    if (*exit_code == 124 || *exit_code == 137)
    {
        OrynLogWarn("QEMU was launched and accepted by the host, but the headless timeout expired.");
        return 1;
    }

    return 0;
}

