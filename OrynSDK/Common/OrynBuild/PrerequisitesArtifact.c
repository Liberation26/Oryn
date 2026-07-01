#include "OrynBuild.h"
#include <stdio.h>
#include <string.h>

static int ResolveSdkRootFromExecutable(const char* executable_path, char* output, size_t output_size)
{
    char bin_dir[ORYN_MAX_PATH];
    char common_dir[ORYN_MAX_PATH];
    if (executable_path == 0 || executable_path[0] == 0)
    {
        snprintf(output, output_size, ".");
        return 0;
    }

    OrynGetDirectoryName(bin_dir, sizeof(bin_dir), executable_path);
    OrynGetDirectoryName(common_dir, sizeof(common_dir), bin_dir);
    OrynGetDirectoryName(output, output_size, common_dir);
    if (output[0] == 0)
    {
        snprintf(output, output_size, ".");
        return 0;
    }
    return 1;
}

static void WriteToolReport(FILE* file, const char* tool)
{
    char path[ORYN_MAX_PATH];
    if (OrynFindProgram(tool, path, sizeof(path)))
    {
        fprintf(file, "[ OK ] %s=%s\n", tool, path);
    }
    else
    {
        fprintf(file, "[WARN] %s=missing\n", tool);
    }
}

void OrynPrereqWriteReport(
    const char* executable_path,
    const char* mode,
    const char* provider_name,
    const char* package_list,
    int missing_count)
{
    char sdk_root[ORYN_MAX_PATH];
    char reports_dir[ORYN_MAX_PATH];
    char report_path[ORYN_MAX_PATH];
    FILE* file;

    ResolveSdkRootFromExecutable(executable_path, sdk_root, sizeof(sdk_root));
    OrynJoinPath(reports_dir, sizeof(reports_dir), sdk_root, "Common/Reports");
    if (!OrynMakeDirectoryRecursive(reports_dir))
    {
        OrynLogWarn("Could not create Common/Reports for prerequisite report.");
        return;
    }

    OrynJoinPath(report_path, sizeof(report_path), reports_dir, "PrerequisitesReport.txt");
    file = fopen(report_path, "w");
    if (file == 0)
    {
        OrynLogWarn("Could not write prerequisite report.");
        return;
    }

    fprintf(file, "Oryn prerequisite report\n");
    fprintf(file, "Version=%s\n", ORYN_VERSION);
    fprintf(file, "Mode=%s\n", mode);
    fprintf(file, "Provider=%s\n", provider_name);
    fprintf(file, "MissingCount=%d\n", missing_count);
    fprintf(file, "PackageList=%s\n", package_list == 0 ? "" : package_list);
    WriteToolReport(file, "clang");
    WriteToolReport(file, "ld.lld");
    WriteToolReport(file, "llvm-ar");
    WriteToolReport(file, "llvm-objcopy");
    WriteToolReport(file, "git");
    WriteToolReport(file, "qemu-system-x86_64");
    WriteToolReport(file, "mkfs.vfat");
    WriteToolReport(file, "mcopy");
    WriteToolReport(file, "zip");
    WriteToolReport(file, "unzip");
    fclose(file);

    char message[ORYN_MAX_PATH + 64];
    snprintf(message, sizeof(message), "Prerequisite report written: %s", report_path);
    OrynLogOk(message);
}
