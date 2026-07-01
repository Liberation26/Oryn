#include "OrynBuild.h"
#include <stdio.h>
#include <string.h>

typedef struct OrynPrereqProvider
{
    char name[32];
    char command[64];
    char install_prefix[256];
    char install_joiner[8];
    int needs_update_step;
} OrynPrereqProvider;

static int FindProviderCommand(const char* tool, char* output, size_t output_size)
{
    if (OrynFindProgram(tool, output, output_size))
    {
        return 1;
    }
    return 0;
}

int OrynPrereqDetectProvider(OrynPrereqProvider* provider)
{
    char found[ORYN_MAX_PATH];
    memset(provider, 0, sizeof(*provider));

    if (FindProviderCommand("apt-get", found, sizeof(found)))
    {
        snprintf(provider->name, sizeof(provider->name), "apt");
        snprintf(provider->command, sizeof(provider->command), "apt-get");
        snprintf(provider->install_prefix, sizeof(provider->install_prefix), "sudo apt-get install -y");
        snprintf(provider->install_joiner, sizeof(provider->install_joiner), " ");
        provider->needs_update_step = 1;
        return 1;
    }

    if (FindProviderCommand("dnf", found, sizeof(found)))
    {
        snprintf(provider->name, sizeof(provider->name), "dnf");
        snprintf(provider->command, sizeof(provider->command), "dnf");
        snprintf(provider->install_prefix, sizeof(provider->install_prefix), "sudo dnf install -y");
        snprintf(provider->install_joiner, sizeof(provider->install_joiner), " ");
        return 1;
    }

    if (FindProviderCommand("pacman", found, sizeof(found)))
    {
        snprintf(provider->name, sizeof(provider->name), "pacman");
        snprintf(provider->command, sizeof(provider->command), "pacman");
        snprintf(provider->install_prefix, sizeof(provider->install_prefix), "sudo pacman -S --needed --noconfirm");
        snprintf(provider->install_joiner, sizeof(provider->install_joiner), " ");
        return 1;
    }

    if (FindProviderCommand("zypper", found, sizeof(found)))
    {
        snprintf(provider->name, sizeof(provider->name), "zypper");
        snprintf(provider->command, sizeof(provider->command), "zypper");
        snprintf(provider->install_prefix, sizeof(provider->install_prefix), "sudo zypper install -y");
        snprintf(provider->install_joiner, sizeof(provider->install_joiner), " ");
        return 1;
    }

    if (FindProviderCommand("brew", found, sizeof(found)))
    {
        snprintf(provider->name, sizeof(provider->name), "brew");
        snprintf(provider->command, sizeof(provider->command), "brew");
        snprintf(provider->install_prefix, sizeof(provider->install_prefix), "brew install");
        snprintf(provider->install_joiner, sizeof(provider->install_joiner), " ");
        return 1;
    }

    if (FindProviderCommand("winget.exe", found, sizeof(found)) || FindProviderCommand("winget", found, sizeof(found)))
    {
        snprintf(provider->name, sizeof(provider->name), "winget");
        snprintf(provider->command, sizeof(provider->command), "winget");
        snprintf(provider->install_prefix, sizeof(provider->install_prefix), "winget install --accept-package-agreements --accept-source-agreements --id");
        snprintf(provider->install_joiner, sizeof(provider->install_joiner), " ");
        return 1;
    }

    return 0;
}

void OrynPrereqProviderInstallCommand(
    const OrynPrereqProvider* provider,
    const char* package_list,
    char* output,
    size_t output_size)
{
    if (strcmp(provider->name, "winget") == 0)
    {
        char packages[2048];
        char* token;
        char* save;
        snprintf(packages, sizeof(packages), "%s", package_list);
        output[0] = 0;
        token = strtok_r(packages, " ", &save);
        while (token != 0)
        {
            if (output[0] != 0)
            {
                strncat(output, " && ", output_size - strlen(output) - 1);
            }
            strncat(output, provider->install_prefix, output_size - strlen(output) - 1);
            strncat(output, " ", output_size - strlen(output) - 1);
            strncat(output, token, output_size - strlen(output) - 1);
            token = strtok_r(0, " ", &save);
        }
        return;
    }

    snprintf(output, output_size, "%s %s", provider->install_prefix, package_list);
}

int OrynPrereqProviderNeedsUpdate(const OrynPrereqProvider* provider)
{
    return provider->needs_update_step;
}

void OrynPrereqProviderUpdateCommand(
    const OrynPrereqProvider* provider,
    char* output,
    size_t output_size)
{
    if (strcmp(provider->name, "apt") == 0)
    {
        snprintf(output, output_size, "sudo apt-get update");
        return;
    }

    output[0] = 0;
}
