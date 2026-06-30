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

static void WriteProfileReportLine(
    FILE* report,
    const OrynProject* project,
    const OrynVmMatrixProfile* profile,
    const char* phase,
    const char* result)
{
    char kernel_file_name[256];
    char kernel_elf[ORYN_MAX_PATH];
    char image_name[256];
    char image_path[ORYN_MAX_PATH];
    char debug_log[ORYN_MAX_PATH];
    char boot_report[ORYN_MAX_PATH];
    char source_snapshot[ORYN_MAX_PATH];
    char build_manifest[ORYN_MAX_PATH];
    char generated_path[ORYN_MAX_PATH];

    OrynMakeKernelElfFileName(kernel_file_name, sizeof(kernel_file_name), project->name);
    OrynJoinPath(kernel_elf, sizeof(kernel_elf), project->build_dir, kernel_file_name);
    snprintf(image_name, sizeof(image_name), "%s.img", project->name);
    OrynJoinPath(image_path, sizeof(image_path), project->output_dir, image_name);
    OrynJoinPath(debug_log, sizeof(debug_log), project->output_dir, "Debug.log");
    OrynJoinPath(boot_report, sizeof(boot_report), project->output_dir, "BootReport.txt");
    BuildSourceSnapshotRoot(project, source_snapshot, sizeof(source_snapshot));
    BuildProfileManifestPath(project, build_manifest, sizeof(build_manifest));
    OrynJoinPath(generated_path, sizeof(generated_path), project->build_dir, "Generated");

    fprintf(report, "%s: %s %s PIC=%s APIC=%s APIC2=%s HPET=%s SMP=%s Display=%s\n",
        result,
        phase,
        profile->Name,
        project->run_pic,
        project->run_apic,
        project->run_apic2,
        project->run_hpet,
        project->run_smp,
        project->run_display);
    fprintf(report, "  Kernel  : %s\n", kernel_elf);
    fprintf(report, "  Image   : %s\n", image_path);
    fprintf(report, "  Source  : %s\n", source_snapshot);
    fprintf(report, "  Manifest: %s\n", build_manifest);
    fprintf(report, "  Generated source/header files: %s\n", generated_path);
    fprintf(report, "  Debug   : %s\n", debug_log);
    fprintf(report, "  Report  : %s\n\n", boot_report);
    fflush(report);
}

static void AppendProfileReport(
    const char* matrix_report_path,
    const OrynProject* project,
    const OrynVmMatrixProfile* profile,
    const char* phase,
    const char* result)
{
    FILE* report = fopen(matrix_report_path, "ab");
    if (report != 0)
    {
        WriteProfileReportLine(report, project, profile, phase, result);
        fclose(report);
    }
}

static void LogProfileSettings(const OrynProject* project, const OrynVmMatrixProfile* profile, unsigned int index)
{
    char message[256];
    snprintf(message, sizeof(message), "Matrix profile %u/%u: %s",
        index + 1U,
        gProfileCount,
        profile->Name);
    OrynLogStep(message);
    OrynLogKeyValue("PIC", project->run_pic);
    OrynLogKeyValue("APIC", project->run_apic);
    OrynLogKeyValue("APIC2/x2APIC", project->run_apic2);
    OrynLogKeyValue("HPET", project->run_hpet);
    OrynLogKeyValue("SMP CPUs", project->run_smp);
    OrynLogKeyValue("Display", project->run_display);
    OrynLogKeyValue("Output mode", MatrixDisplayIsInteractive(project->run_display) ? "graphical framebuffer screen" : "headless terminal serial");
    OrynLogKeyValue("Build", project->build_dir);
    OrynLogKeyValue("Output", project->output_dir);
}

static void AppendMatrixSection(const char* matrix_report_path, const char* title)
{
    FILE* report = fopen(matrix_report_path, "ab");
    if (report != 0)
    {
        fprintf(report, "%s\n", title);
        fclose(report);
    }
}

static int OrynCommandMatrixInternal(
    const char* executable_path,
    const char* project_file,
    const char* matrix_folder,
    const char* display_mode,
    const char* mode_title)
{
    OrynProject base_project;
    char build_run_root[ORYN_MAX_PATH];
    char output_run_root[ORYN_MAX_PATH];
    char run_id[128];
    char matrix_report_path[ORYN_MAX_PATH];
    int build_ok[16];
    int test_ok[16];
    int built = 0;
    int build_failed = 0;
    int tested = 0;
    int test_failed = 0;
    int test_skipped = 0;
    int interactive_mode = MatrixDisplayIsInteractive(display_mode);

    for (unsigned int index = 0; index < gProfileCount; ++index)
    {
        build_ok[index] = 0;
        test_ok[index] = 0;
    }

    if (!OrynLoadProject(executable_path, project_file, &base_project))
    {
        return 1;
    }

    ResolveUniqueRunRoots(&base_project, matrix_folder, build_run_root, sizeof(build_run_root),
        output_run_root, sizeof(output_run_root), run_id, sizeof(run_id));
    OrynJoinPath(matrix_report_path, sizeof(matrix_report_path), output_run_root, "MatrixReport.txt");

    FILE* report = fopen(matrix_report_path, "wb");
    if (report == 0)
    {
        OrynLogFail("Could not create matrix report.");
        return 1;
    }

    fprintf(report, "Oryn VM interrupt/timer matrix report\n");
    fprintf(report, "Version: %s\n", ORYN_VERSION);
    fprintf(report, "Project: %s\n", base_project.name);
    fprintf(report, "Run: %s\n", run_id);
    fprintf(report, "Mode: %s\n", mode_title);
    fprintf(report, "Display: %s\n", display_mode);
    fprintf(report, "Output stream: %s\n", interactive_mode ?
        "graphical framebuffer screen in a QEMU window" :
        "headless terminal serial/debug output, not the graphical framebuffer");
    fprintf(report, "Every profile keeps its own kernel, image, source snapshot, manifest, generated build files, debug log, and boot report.\n");
    fprintf(report, "Build root: %s\n", build_run_root);
    fprintf(report, "Output root: %s\n\n", output_run_root);
    fclose(report);

    OrynLogStep("Building every PIC/APIC/APIC2/HPET matrix kernel first.");
    OrynLogInfo("Each profile prints an explicit start line, keeps its compiler/linker commands visible, and flushes logs immediately.");
    OrynLogKeyValue("Matrix run", run_id);
    OrynLogKeyValue("Matrix mode", mode_title);
    OrynLogKeyValue("Matrix display", display_mode);
    OrynLogKeyValue("Matrix build root", build_run_root);
    OrynLogKeyValue("Matrix output root", output_run_root);
    AppendMatrixSection(matrix_report_path, "Build phase:");

    for (unsigned int index = 0; index < gProfileCount; ++index)
    {
        OrynProject profile_project;
        int ok = 1;
        char build_start[256];

        ConfigureProfileProject(&profile_project, &base_project, &gProfiles[index],
            build_run_root, output_run_root, display_mode);
        LogProfileSettings(&profile_project, &gProfiles[index], index);
        snprintf(build_start, sizeof(build_start), "Starting build phase for profile %s. Nothing is shared or overwritten.", gProfiles[index].Name);
        OrynLogStep(build_start);

        if (!SnapshotProfileSources(&profile_project, &gProfiles[index]))
        {
            ok = 0;
        }
        if (ok && !OrynBuildKernel(&profile_project))
        {
            ok = 0;
        }
        if (ok && !OrynBuildImage(&profile_project))
        {
            ok = 0;
        }

        build_ok[index] = ok;
        AppendProfileReport(matrix_report_path, &profile_project, &gProfiles[index],
            "BUILD", ok ? "PASS" : "FAIL");

        if (ok)
        {
            built += 1;
            OrynLogOk("Matrix profile built and kept in its own folder with its own source snapshot.");
        }
        else
        {
            build_failed += 1;
            OrynLogFail("Matrix profile build failed. Its folder, source snapshot, manifest, and any partial output were kept for diagnostics.");
        }
    }

    OrynLogStep(interactive_mode ?
        "Testing built matrix kernels in graphical QEMU framebuffer windows." :
        "Testing built matrix kernels in headless QEMU serial mode.");
    if (interactive_mode)
    {
        OrynLogInfo("This mode is graphical screen/framebuffer output. It is not live terminal serial output.");
        OrynLogInfo("QEMU opens one window per profile. Close the QEMU window after checking keyboard and scrolling to continue to the next profile.");
    }
    else
    {
        OrynLogInfo("This mode is live terminal serial/debug output. It is not the graphical framebuffer screen.");
        OrynLogInfo("Each headless kernel requests QEMU debug-exit after its proof tasks complete; a timeout fails the profile instead of hanging forever.");
    }
    AppendMatrixSection(matrix_report_path, "Test phase:");

    for (unsigned int index = 0; index < gProfileCount; ++index)
    {
        OrynProject profile_project;

        ConfigureProfileProject(&profile_project, &base_project, &gProfiles[index],
            build_run_root, output_run_root, display_mode);

        if (!build_ok[index])
        {
            test_skipped += 1;
            AppendProfileReport(matrix_report_path, &profile_project, &gProfiles[index],
                "TEST", "SKIP");
            OrynLogWarn("Skipping QEMU test because this profile did not build.");
            continue;
        }

        LogProfileSettings(&profile_project, &gProfiles[index], index);
        test_ok[index] = OrynRunQemu(&profile_project);
        AppendProfileReport(matrix_report_path, &profile_project, &gProfiles[index],
            "TEST", test_ok[index] ? "PASS" : "FAIL");

        if (test_ok[index])
        {
            tested += 1;
            OrynLogOk(interactive_mode ?
                "Matrix profile passed its graphical screen/framebuffer QEMU test after the window closed." :
                "Matrix profile passed its headless serial QEMU test and exited QEMU.");
        }
        else
        {
            test_failed += 1;
            OrynLogFail("Matrix profile QEMU test failed. Its kernel, image, source snapshot, debug log, and boot report were kept.");
        }
    }

    report = fopen(matrix_report_path, "ab");
    if (report != 0)
    {
        fprintf(report, "Summary: %d built, %d build failed, %d tested, %d test failed, %d test skipped\n",
            built, build_failed, tested, test_failed, test_skipped);
        fclose(report);
    }

    OrynLogKeyValue("Matrix report", matrix_report_path);
    snprintf(run_id, sizeof(run_id), "%d built, %d build failed, %d tested, %d test failed, %d test skipped",
        built, build_failed, tested, test_failed, test_skipped);
    if (build_failed == 0 && test_failed == 0 && test_skipped == 0)
    {
        OrynLogOk(run_id);
        return 0;
    }

    OrynLogFail(run_id);
    return 1;
}

int OrynCommandMatrix(const char* executable_path, const char* project_file)
{
    return OrynCommandMatrixInternal(executable_path, project_file, "Matrix", "none",
        "headless serial matrix - live terminal serial/debug output only");
}

int OrynCommandMatrixSerial(const char* executable_path, const char* project_file)
{
    return OrynCommandMatrix(executable_path, project_file);
}

int OrynCommandMatrixScreen(const char* executable_path, const char* project_file)
{
    return OrynCommandMatrixInternal(executable_path, project_file, "MatrixScreen", "sdl",
        "graphical screen matrix - QEMU framebuffer window output");
}

int OrynCommandMatrixAll(const char* executable_path, const char* project_file)
{
    int serial_result;
    int screen_result;

    serial_result = OrynCommandMatrixSerial(executable_path, project_file);
    screen_result = OrynCommandMatrixScreen(executable_path, project_file);
    return (serial_result == 0 && screen_result == 0) ? 0 : 1;
}
