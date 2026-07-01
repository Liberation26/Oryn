#include "OrynBuild.h"
#include <stdio.h>
#include <string.h>

typedef struct OrynPrerequisite OrynPrerequisite;
typedef struct OrynPrereqProvider OrynPrereqProvider;

struct OrynPrerequisite
{
    const char* tool;
    const char* purpose;
    const char* apt_package;
    const char* dnf_package;
    const char* pacman_package;
    const char* zypper_package;
    const char* brew_package;
    const char* winget_package;
    int required;
};

struct OrynPrereqProvider
{
    char name[32];
    char command[64];
    char install_prefix[256];
    char install_joiner[8];
    int needs_update_step;
};

const OrynPrerequisite* OrynPrereqTable(int* count);
const char* OrynPrereqPackageForProvider(const OrynPrerequisite* item, const char* provider);
int OrynPrereqDetectProvider(OrynPrereqProvider* provider);
void OrynPrereqProviderInstallCommand(const OrynPrereqProvider* provider, const char* package_list, char* output, size_t output_size);
int OrynPrereqProviderNeedsUpdate(const OrynPrereqProvider* provider);
void OrynPrereqProviderUpdateCommand(const OrynPrereqProvider* provider, char* output, size_t output_size);

static void AddUniquePackage(char packages[64][128], int* count, const char* package_name)
{
    if (package_name == 0 || package_name[0] == 0)
    {
        return;
    }

    for (int index = 0; index < *count; ++index)
    {
        if (strcmp(packages[index], package_name) == 0)
        {
            return;
        }
    }

    if (*count < 64)
    {
        snprintf(packages[*count], 128, "%s", package_name);
        *count += 1;
    }
}

static void AddProviderPackageByName(char packages[64][128], int* count, const char* provider_name, const char* apt, const char* dnf, const char* pacman, const char* zypper, const char* brew, const char* winget)
{
    if (strcmp(provider_name, "apt") == 0) AddUniquePackage(packages, count, apt);
    else if (strcmp(provider_name, "dnf") == 0) AddUniquePackage(packages, count, dnf);
    else if (strcmp(provider_name, "pacman") == 0) AddUniquePackage(packages, count, pacman);
    else if (strcmp(provider_name, "zypper") == 0) AddUniquePackage(packages, count, zypper);
    else if (strcmp(provider_name, "brew") == 0) AddUniquePackage(packages, count, brew);
    else if (strcmp(provider_name, "winget") == 0) AddUniquePackage(packages, count, winget);
}

static void JoinPackages(char packages[64][128], int count, char* output, size_t output_size)
{
    output[0] = 0;
    for (int index = 0; index < count; ++index)
    {
        if (index > 0)
        {
            strncat(output, " ", output_size - strlen(output) - 1);
        }
        strncat(output, packages[index], output_size - strlen(output) - 1);
    }
}

static void PrintEnvironmentNotes(void)
{
#if defined(_WIN32)
    OrynLogInfo("Host OS compile target: Windows.");
#elif defined(__APPLE__)
    OrynLogInfo("Host OS compile target: macOS.");
#elif defined(__linux__)
    OrynLogInfo("Host OS compile target: Linux/WSL.");
#else
    OrynLogWarn("Host OS compile target is unknown; provider detection will still be attempted.");
#endif
}

static int CheckWindowsSideTools(void)
{
    int ok = 1;
    char qemu[ORYN_MAX_PATH];
    char ovmf[ORYN_MAX_PATH];
    if (OrynFindWindowsQemu(qemu, sizeof(qemu)))
    {
        char message[ORYN_MAX_PATH + 64];
        snprintf(message, sizeof(message), "Windows QEMU found: %s", qemu);
        OrynLogOk(message);
    }
    else
    {
        OrynLogWarn("Windows QEMU was not found; native qemu-system-x86_64 will be used if present.");
    }

    if (OrynFindOvmf(ovmf, sizeof(ovmf)))
    {
        char message[ORYN_MAX_PATH + 64];
        snprintf(message, sizeof(message), "OVMF found: %s", ovmf);
        OrynLogOk(message);
    }
    else
    {
        OrynLogFail("OVMF firmware was not found. Set ORYN_OVMF_PATH or install QEMU/OVMF.");
        ok = 0;
    }
    return ok;
}

void OrynPrereqWriteReport(const char* executable_path, const char* mode, const char* provider_name, const char* package_list, int missing_count);

int OrynPrereqRun(const char* executable_path, const char* mode)
{
    int table_count = 0;
    int missing_count = 0;
    char missing_packages[64][128];
    const OrynPrerequisite* table = OrynPrereqTable(&table_count);
    OrynPrereqProvider provider;
    int provider_found = OrynPrereqDetectProvider(&provider);

    memset(missing_packages, 0, sizeof(missing_packages));
    OrynLogStep("Resolving Oryn host prerequisites.");
    PrintEnvironmentNotes();

    if (provider_found)
    {
        char message[128];
        snprintf(message, sizeof(message), "Package provider: %s", provider.name);
        OrynLogOk(message);
    }
    else
    {
        OrynLogWarn("No supported package provider was detected.");
    }

    for (int index = 0; index < table_count; ++index)
    {
        char path[ORYN_MAX_PATH];
        if (OrynFindProgram(table[index].tool, path, sizeof(path)))
        {
            char message[ORYN_MAX_PATH + 96];
            snprintf(message, sizeof(message), "%s found: %s", table[index].tool, path);
            OrynLogOk(message);
        }
        else
        {
            char message[512];
            const char* package_name = provider_found ? OrynPrereqPackageForProvider(&table[index], provider.name) : "";
            snprintf(message, sizeof(message), "%s missing. Purpose: %s", table[index].tool, table[index].purpose);
            OrynLogWarn(message);
            AddUniquePackage(missing_packages, &missing_count, package_name);
        }
    }

    if (!CheckWindowsSideTools())
    {
        if (provider_found)
        {
            AddProviderPackageByName(missing_packages, &missing_count, provider.name, "ovmf", "edk2-ovmf", "edk2-ovmf", "ovmf", "qemu", "SoftwareFreedomConservancy.QEMU");
        }
        else
        {
            missing_count += 1;
        }
    }

    if (strcmp(mode, "manifest") == 0)
    {
        OrynLogInfo("Prerequisite manifest: clang, lld, llvm-ar, llvm-objcopy, git, QEMU, OVMF, FAT tools, zip/unzip.");
    }

    char package_list[2048];
    JoinPackages(missing_packages, missing_count, package_list, sizeof(package_list));
    OrynPrereqWriteReport(executable_path, mode, provider_found ? provider.name : "none", package_list, missing_count);

    if (missing_count == 0)
    {
        OrynLogOk("All prerequisites are filled.");
        return 0;
    }

    if (!provider_found)
    {
        OrynLogFail("Prerequisites are missing and no installer provider is available.");
        return 1;
    }

    char command[3072];

    if (package_list[0] == 0)
    {
        OrynLogFail("Some missing tools do not have an installer package for this provider.");
        return 1;
    }

    OrynPrereqProviderInstallCommand(&provider, package_list, command, sizeof(command));
    if (strcmp(mode, "install") == 0)
    {
        if (OrynPrereqProviderNeedsUpdate(&provider))
        {
            char update_command[512];
            OrynPrereqProviderUpdateCommand(&provider, update_command, sizeof(update_command));
            if (update_command[0] != 0 && !OrynRunCommand(update_command))
            {
                OrynLogFail("Package provider update failed.");
                return 1;
            }
        }
        if (!OrynRunCommand(command))
        {
            OrynLogFail("Prerequisite installation command failed.");
            return 1;
        }
        OrynLogOk("Prerequisite installation command completed. Run 'oryn prerequisites check' to verify.");
        return 0;
    }

    OrynLogInfo("Install command that would be run:");
    OrynLogCommand(command);
    OrynLogFail("Prerequisites are not completely filled yet.");
    return 1;
}
