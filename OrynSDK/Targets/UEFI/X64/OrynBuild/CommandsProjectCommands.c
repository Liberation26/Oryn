#include "TargetBuildInternal.h"
int OrynCommandDoctor(const char* executable_path)
{
    (void)executable_path;
    OrynLogStep("Checking Oryn WSL development tools.");

    int ok = 1;
    PrintToolStatus("clang", "clang", &ok);
    PrintToolStatus("ld.lld", "ld.lld", &ok);
    PrintToolStatus("lld-link", "lld-link", &ok);
    PrintToolStatus("llvm-objcopy", "llvm-objcopy", &ok);
    PrintToolStatus("wslpath", "wslpath", &ok);
    PrintWindowsQemuStatus(&ok);

    char ovmf[ORYN_MAX_PATH];
    if (OrynFindOvmf(ovmf, sizeof(ovmf)))
    {
        char message[ORYN_MAX_PATH + 128];
        snprintf(message, sizeof(message), "OVMF found: %s", ovmf);
        OrynLogOk(message);
    }
    else
    {
        OrynLogFail("OVMF was not found. Expected Windows QEMU OVMF under C:/Program Files/qemu/share, WSL ovmf, or ORYN_OVMF_PATH.");
        ok = 0;
    }

    if (ok)
    {
        OrynLogOk("Doctor checks passed.");
        return 0;
    }

    OrynLogFail("Doctor checks failed.");
    return 1;
}

int OrynCommandBuild(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    OrynLogKeyValue("Project", project.name);
    OrynLogKeyValue("Target", project.target);
    OrynLogKeyValue("Toolchain", project.toolchain);

    return OrynBuildKernel(&project) ? 0 : 1;
}

int OrynCommandImage(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    if (!OrynBuildKernel(&project))
    {
        return 1;
    }

    return OrynBuildImage(&project) ? 0 : 1;
}

int OrynCommandRun(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    if (!OrynBuildKernel(&project))
    {
        return 1;
    }

    char run_image[ORYN_MAX_PATH];
    BuildProjectImagePath(&project, run_image, sizeof(run_image));
    if (ProjectBoolEnabled(project.run_format_vm, 1) || !OrynFileExists(run_image))
    {
        if (!OrynBuildImage(&project))
        {
            return 1;
        }
    }
    else
    {
        OrynLogInfo("VMSettings FormatVM=no; reusing existing VM disk image.");
    }

    return OrynRunQemu(&project) ? 0 : 1;
}

int OrynCommandClean(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    return OrynCleanProject(&project) ? 0 : 1;
}

