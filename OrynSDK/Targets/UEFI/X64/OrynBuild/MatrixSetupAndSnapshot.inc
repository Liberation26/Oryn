#include "OrynBuild.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

typedef struct OrynVmMatrixProfile
{
    const char* Name;
    int Pic;
    int Apic;
    int Apic2;
    int Hpet;
} OrynVmMatrixProfile;

static const OrynVmMatrixProfile gProfiles[] =
{
    { "NONE", 0, 0, 0, 0 },
    { "PIC", 1, 0, 0, 0 },
    { "APIC", 0, 1, 0, 0 },
    { "APIC2", 0, 0, 1, 0 },
    { "HPET", 0, 0, 0, 1 },
    { "PIC_APIC", 1, 1, 0, 0 },
    { "PIC_APIC2", 1, 0, 1, 0 },
    { "PIC_HPET", 1, 0, 0, 1 },
    { "APIC_APIC2", 0, 1, 1, 0 },
    { "APIC_HPET", 0, 1, 0, 1 },
    { "APIC2_HPET", 0, 0, 1, 1 },
    { "PIC_APIC_APIC2", 1, 1, 1, 0 },
    { "PIC_APIC_HPET", 1, 1, 0, 1 },
    { "PIC_APIC2_HPET", 1, 0, 1, 1 },
    { "APIC_APIC2_HPET", 0, 1, 1, 1 },
    { "PIC_APIC_APIC2_HPET", 1, 1, 1, 1 }
};

static const unsigned int gProfileCount = sizeof(gProfiles) / sizeof(gProfiles[0]);

static const char* OnOff(int value)
{
    return value ? "on" : "off";
}

static void SetOnOff(char* output, size_t output_size, int value)
{
    snprintf(output, output_size, "%s", OnOff(value));
}

static int MatrixTextEqualsIgnoreCase(const char* left, const char* right)
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

static int MatrixDisplayIsInteractive(const char* display_mode)
{
    if (display_mode == 0 || display_mode[0] == 0)
    {
        return 0;
    }

    return !MatrixTextEqualsIgnoreCase(display_mode, "none") &&
        !MatrixTextEqualsIgnoreCase(display_mode, "headless");
}

static void MakeMatrixRunId(char* output, size_t output_size)
{
    time_t now = time(0);
    struct tm* parts = localtime(&now);
    if (parts != 0 && strftime(output, output_size, "Run-%Y%m%d-%H%M%S", parts) != 0)
    {
        return;
    }

    snprintf(output, output_size, "Run-unknown-time");
}

static void ResolveUniqueRunRoots(
    const OrynProject* project,
    const char* matrix_folder,
    char* build_run_root,
    size_t build_run_root_size,
    char* output_run_root,
    size_t output_run_root_size,
    char* run_id,
    size_t run_id_size)
{
    char build_matrix_root[ORYN_MAX_PATH];
    char output_matrix_root[ORYN_MAX_PATH];
    char base_run_id[128];

    char build_matrix_folder[256];
    char output_matrix_folder[256];
    snprintf(build_matrix_folder, sizeof(build_matrix_folder), "Build/%s", matrix_folder);
    snprintf(output_matrix_folder, sizeof(output_matrix_folder), "Output/%s", matrix_folder);
    OrynJoinPath(build_matrix_root, sizeof(build_matrix_root), project->project_root, build_matrix_folder);
    OrynJoinPath(output_matrix_root, sizeof(output_matrix_root), project->project_root, output_matrix_folder);
    OrynMakeDirectoryRecursive(build_matrix_root);
    OrynMakeDirectoryRecursive(output_matrix_root);

    MakeMatrixRunId(base_run_id, sizeof(base_run_id));
    for (int suffix = 0; suffix < 1000; ++suffix)
    {
        if (suffix == 0)
        {
            snprintf(run_id, run_id_size, "%s", base_run_id);
        }
        else
        {
            snprintf(run_id, run_id_size, "%s-%03d", base_run_id, suffix);
        }

        OrynJoinPath(build_run_root, build_run_root_size, build_matrix_root, run_id);
        OrynJoinPath(output_run_root, output_run_root_size, output_matrix_root, run_id);
        if (!OrynPathExists(build_run_root) && !OrynPathExists(output_run_root))
        {
            OrynMakeDirectoryRecursive(build_run_root);
            OrynMakeDirectoryRecursive(output_run_root);
            return;
        }
    }

    OrynMakeDirectoryRecursive(build_run_root);
    OrynMakeDirectoryRecursive(output_run_root);
}

static void ConfigureProfileProject(
    OrynProject* profile_project,
    const OrynProject* base_project,
    const OrynVmMatrixProfile* profile,
    const char* build_run_root,
    const char* output_run_root,
    const char* display_mode)
{
    *profile_project = *base_project;
    SetOnOff(profile_project->run_pic, sizeof(profile_project->run_pic), profile->Pic);
    SetOnOff(profile_project->run_apic, sizeof(profile_project->run_apic), profile->Apic);
    SetOnOff(profile_project->run_apic2, sizeof(profile_project->run_apic2), profile->Apic2);
    SetOnOff(profile_project->run_hpet, sizeof(profile_project->run_hpet), profile->Hpet);
    snprintf(profile_project->run_display, sizeof(profile_project->run_display), "%s", display_mode);
    snprintf(profile_project->run_format_vm, sizeof(profile_project->run_format_vm), "yes");

    if (!profile->Apic && !profile->Apic2)
    {
        snprintf(profile_project->run_smp, sizeof(profile_project->run_smp), "1");
    }

    OrynJoinPath(profile_project->build_dir, sizeof(profile_project->build_dir), build_run_root, profile->Name);
    OrynJoinPath(profile_project->object_dir, sizeof(profile_project->object_dir), profile_project->build_dir, "Objects");
    OrynJoinPath(profile_project->output_dir, sizeof(profile_project->output_dir), output_run_root, profile->Name);
    OrynJoinPath(profile_project->esp_dir, sizeof(profile_project->esp_dir), profile_project->output_dir, "ESP");
}

static int SnapshotShouldSkip(const char* name)
{
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
    {
        return 1;
    }

    return 0;
}

static int CopyDirectorySnapshotRecursive(const char* source, const char* target)
{
    if (source == 0 || source[0] == 0 || !OrynPathExists(source))
    {
        return 1;
    }

    if (OrynFileExists(source))
    {
        return OrynCopyFile(source, target);
    }

    if (!OrynDirectoryExists(source))
    {
        return 1;
    }

    if (!OrynMakeDirectoryRecursive(target))
    {
        return 0;
    }

    DIR* directory = opendir(source);
    if (directory == 0)
    {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(directory)) != 0)
    {
        if (SnapshotShouldSkip(entry->d_name))
        {
            continue;
        }

        char source_child[ORYN_MAX_PATH];
        char target_child[ORYN_MAX_PATH];
        OrynJoinPath(source_child, sizeof(source_child), source, entry->d_name);
        OrynJoinPath(target_child, sizeof(target_child), target, entry->d_name);

        if (!CopyDirectorySnapshotRecursive(source_child, target_child))
        {
            closedir(directory);
            return 0;
        }
    }

    closedir(directory);
    return 1;
}

static int CopySnapshotPath(const char* snapshot_root, const char* relative_target, const char* source)
{
    char target[ORYN_MAX_PATH];
    OrynJoinPath(target, sizeof(target), snapshot_root, relative_target);
    return CopyDirectorySnapshotRecursive(source, target);
}

static void BuildSourceSnapshotRoot(const OrynProject* project, char* output, size_t output_size)
{
    OrynJoinPath(output, output_size, project->build_dir, "SourceSnapshot");
}

static void BuildProfileManifestPath(const OrynProject* project, char* output, size_t output_size)
{
    OrynJoinPath(output, output_size, project->build_dir, "BuildManifest.txt");
}

static int WriteProfileBuildManifest(const OrynProject* project, const OrynVmMatrixProfile* profile)
{
    char manifest_path[ORYN_MAX_PATH];
    char source_snapshot[ORYN_MAX_PATH];
    char generated_path[ORYN_MAX_PATH];

    BuildProfileManifestPath(project, manifest_path, sizeof(manifest_path));
    BuildSourceSnapshotRoot(project, source_snapshot, sizeof(source_snapshot));
    OrynJoinPath(generated_path, sizeof(generated_path), project->build_dir, "Generated");

    char manifest_directory[ORYN_MAX_PATH];
    OrynGetDirectoryName(manifest_directory, sizeof(manifest_directory), manifest_path);
    OrynMakeDirectoryRecursive(manifest_directory);

    FILE* manifest = fopen(manifest_path, "wb");
    if (manifest == 0)
    {
        return 0;
    }

    fprintf(manifest, "Oryn matrix kernel build manifest\n");
    fprintf(manifest, "Version: %s\n", ORYN_VERSION);
    fprintf(manifest, "Project: %s\n", project->name);
    fprintf(manifest, "Profile: %s\n", profile->Name);
    fprintf(manifest, "PIC: %s\n", project->run_pic);
    fprintf(manifest, "APIC: %s\n", project->run_apic);
    fprintf(manifest, "APIC2/x2APIC: %s\n", project->run_apic2);
    fprintf(manifest, "HPET: %s\n", project->run_hpet);
    fprintf(manifest, "SMP CPUs: %s\n", project->run_smp);
    fprintf(manifest, "Display: %s\n", project->run_display);
    fprintf(manifest, "Compile define ORYN_VM_PIC: %d\n", profile->Pic);
    fprintf(manifest, "Compile define ORYN_VM_APIC: %d\n", profile->Apic);
    fprintf(manifest, "Compile define ORYN_VM_APIC2: %d\n", profile->Apic2);
    fprintf(manifest, "Compile define ORYN_VM_HPET: %d\n", profile->Hpet);
    fprintf(manifest, "Compile define ORYN_VM_SMP_CPUS: %s\n", project->run_smp);
    fprintf(manifest, "Compile define ORYN_VM_INTERACTIVE_DISPLAY: %d\n", MatrixDisplayIsInteractive(project->run_display));
    fprintf(manifest, "Build folder: %s\n", project->build_dir);
    fprintf(manifest, "Objects folder: %s\n", project->object_dir);
    fprintf(manifest, "Output folder: %s\n", project->output_dir);
    fprintf(manifest, "Source snapshot: %s\n", source_snapshot);
    fprintf(manifest, "Generated build source/header files: %s\n", generated_path);
    fprintf(manifest, "Original project file: %s\n", project->project_file);
    fprintf(manifest, "Original project source: %s\n", project->source_dir);
    fprintf(manifest, "Original project include: %s\n", project->include_dir);
    fprintf(manifest, "Original selected kernel folder: %s\n", project->selected_kernel_dir);
    fprintf(manifest, "Original SDK common kernel include: %s\n", project->sdk_kernel_common_include_dir);
    fprintf(manifest, "Original SDK common kernel source: %s\n", project->sdk_kernel_common_source_dir);
    fprintf(manifest, "Original SDK target kernel include: %s\n", project->sdk_kernel_target_include_dir);
    fprintf(manifest, "Original SDK target kernel source: %s\n", project->sdk_kernel_target_source_dir);
    fclose(manifest);
    return 1;
}

static int SnapshotProfileSources(const OrynProject* project, const OrynVmMatrixProfile* profile)
{
    (void)profile;

    char snapshot_root[ORYN_MAX_PATH];
    char sdk_common_handoff_include[ORYN_MAX_PATH];
    char sdk_libc_include[ORYN_MAX_PATH];
    char sdk_libc_source[ORYN_MAX_PATH];
    char target_boot_include[ORYN_MAX_PATH];
    char target_loader_include[ORYN_MAX_PATH];
    char target_loader_source[ORYN_MAX_PATH];
    char target_kernel_ld[ORYN_MAX_PATH];
    char project_file_target[ORYN_MAX_PATH];
    char kernel_items_source[ORYN_MAX_PATH];
    char kernel_items_target[ORYN_MAX_PATH];
    char kernel_selected_source[ORYN_MAX_PATH];
    char kernel_selected_target[ORYN_MAX_PATH];
    char selected_number_text[64];

    BuildSourceSnapshotRoot(project, snapshot_root, sizeof(snapshot_root));
    OrynMakeDirectoryRecursive(snapshot_root);

    OrynJoinPath(project_file_target, sizeof(project_file_target), snapshot_root, "Project/Project.oryn");
    if (!OrynCopyFile(project->project_file, project_file_target))
    {
        OrynLogFail("Could not copy Project.oryn into the matrix source snapshot.");
        return 0;
    }

    if (!CopySnapshotPath(snapshot_root, "Project/Source", project->source_dir) ||
        !CopySnapshotPath(snapshot_root, "Project/Include", project->include_dir))
    {
        OrynLogFail("Could not copy project source/include into the matrix source snapshot.");
        return 0;
    }

    OrynJoinPath(kernel_items_source, sizeof(kernel_items_source), project->kernel_variants_root, "BootInfoItems.txt");
    OrynJoinPath(kernel_items_target, sizeof(kernel_items_target), snapshot_root, "Project/Kernel/BootInfoItems.txt");
    if (OrynFileExists(kernel_items_source) && !OrynCopyFile(kernel_items_source, kernel_items_target))
    {
        OrynLogFail("Could not copy BootInfoItems.txt into the matrix source snapshot.");
        return 0;
    }

    OrynJoinPath(kernel_selected_source, sizeof(kernel_selected_source), project->kernel_variants_root, "Selected.txt");
    OrynJoinPath(kernel_selected_target, sizeof(kernel_selected_target), snapshot_root, "Project/Kernel/Selected.txt");
    if (OrynFileExists(kernel_selected_source) && !OrynCopyFile(kernel_selected_source, kernel_selected_target))
    {
        OrynLogFail("Could not copy Selected.txt into the matrix source snapshot.");
        return 0;
    }

    if (project->selected_kernel_dir[0] != 0)
    {
        snprintf(selected_number_text, sizeof(selected_number_text), "Project/Kernel/%d", project->selected_kernel_number);
        if (!CopySnapshotPath(snapshot_root, selected_number_text, project->selected_kernel_dir))
        {
            OrynLogFail("Could not copy selected Kernel/<n> source into the matrix source snapshot.");
            return 0;
        }
    }

    OrynJoinPath(sdk_common_handoff_include, sizeof(sdk_common_handoff_include), project->sdk_root, "Common/Handoff/Include");
    OrynJoinPath(sdk_libc_include, sizeof(sdk_libc_include), project->sdk_root, "Common/OrynLibC/Include");
    OrynJoinPath(sdk_libc_source, sizeof(sdk_libc_source), project->sdk_root, "Common/OrynLibC/Source");
    OrynJoinPath(target_boot_include, sizeof(target_boot_include), project->sdk_root, "Targets/UEFI/X64/Boot/Include");
    OrynJoinPath(target_loader_include, sizeof(target_loader_include), project->sdk_root, "Targets/UEFI/X64/Loader/Include");
    OrynJoinPath(target_loader_source, sizeof(target_loader_source), project->sdk_root, "Targets/UEFI/X64/Loader/Source");
    OrynJoinPath(target_kernel_ld, sizeof(target_kernel_ld), project->sdk_root, "Targets/UEFI/X64/Kernel.ld");

    if (!CopySnapshotPath(snapshot_root, "SDK/Common/Kernel/Include", project->sdk_kernel_common_include_dir) ||
        !CopySnapshotPath(snapshot_root, "SDK/Common/Kernel/Source", project->sdk_kernel_common_source_dir) ||
        !CopySnapshotPath(snapshot_root, "SDK/Common/Handoff/Include", sdk_common_handoff_include) ||
        !CopySnapshotPath(snapshot_root, "SDK/Common/OrynLibC/Include", sdk_libc_include) ||
        !CopySnapshotPath(snapshot_root, "SDK/Common/OrynLibC/Source", sdk_libc_source) ||
        !CopySnapshotPath(snapshot_root, "SDK/Targets/UEFI/X64/KernelSupport/Include", project->sdk_kernel_target_include_dir) ||
        !CopySnapshotPath(snapshot_root, "SDK/Targets/UEFI/X64/KernelSupport/Source", project->sdk_kernel_target_source_dir) ||
        !CopySnapshotPath(snapshot_root, "SDK/Targets/UEFI/X64/Boot/Include", target_boot_include) ||
        !CopySnapshotPath(snapshot_root, "SDK/Targets/UEFI/X64/Loader/Include", target_loader_include) ||
        !CopySnapshotPath(snapshot_root, "SDK/Targets/UEFI/X64/Loader/Source", target_loader_source) ||
        !CopySnapshotPath(snapshot_root, "SDK/Targets/UEFI/X64/Kernel.ld", target_kernel_ld))
    {
        OrynLogFail("Could not copy SDK kernel/loader source into the matrix source snapshot.");
        return 0;
    }

    if (!WriteProfileBuildManifest(project, profile))
    {
        OrynLogFail("Could not write BuildManifest.txt for this matrix profile.");
        return 0;
    }

    OrynLogKeyValue("Source snapshot", snapshot_root);
    char manifest_path[ORYN_MAX_PATH];
    BuildProfileManifestPath(project, manifest_path, sizeof(manifest_path));
    OrynLogKeyValue("Build manifest", manifest_path);
    return 1;
}

