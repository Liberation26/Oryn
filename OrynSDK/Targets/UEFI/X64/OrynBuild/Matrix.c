#include "OrynBuild.h"
#include <stdio.h>
#include <string.h>
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

    OrynJoinPath(build_matrix_root, sizeof(build_matrix_root), project->project_root, "Build/Matrix");
    OrynJoinPath(output_matrix_root, sizeof(output_matrix_root), project->project_root, "Output/Matrix");
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
    const char* output_run_root)
{
    *profile_project = *base_project;
    SetOnOff(profile_project->run_pic, sizeof(profile_project->run_pic), profile->Pic);
    SetOnOff(profile_project->run_apic, sizeof(profile_project->run_apic), profile->Apic);
    SetOnOff(profile_project->run_apic2, sizeof(profile_project->run_apic2), profile->Apic2);
    SetOnOff(profile_project->run_hpet, sizeof(profile_project->run_hpet), profile->Hpet);
    snprintf(profile_project->run_display, sizeof(profile_project->run_display), "none");
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

    OrynMakeKernelElfFileName(kernel_file_name, sizeof(kernel_file_name), project->name);
    OrynJoinPath(kernel_elf, sizeof(kernel_elf), project->build_dir, kernel_file_name);
    snprintf(image_name, sizeof(image_name), "%s.img", project->name);
    OrynJoinPath(image_path, sizeof(image_path), project->output_dir, image_name);
    OrynJoinPath(debug_log, sizeof(debug_log), project->output_dir, "Debug.log");
    OrynJoinPath(boot_report, sizeof(boot_report), project->output_dir, "BootReport.txt");

    fprintf(report, "%s: %s %s PIC=%s APIC=%s APIC2=%s HPET=%s SMP=%s\n",
        result,
        phase,
        profile->Name,
        project->run_pic,
        project->run_apic,
        project->run_apic2,
        project->run_hpet,
        project->run_smp);
    fprintf(report, "  Kernel: %s\n", kernel_elf);
    fprintf(report, "  Image : %s\n", image_path);
    fprintf(report, "  Debug : %s\n", debug_log);
    fprintf(report, "  Report: %s\n\n", boot_report);
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

int OrynCommandMatrix(const char* executable_path, const char* project_file)
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

    for (unsigned int index = 0; index < gProfileCount; ++index)
    {
        build_ok[index] = 0;
        test_ok[index] = 0;
    }

    if (!OrynLoadProject(executable_path, project_file, &base_project))
    {
        return 1;
    }

    ResolveUniqueRunRoots(&base_project, build_run_root, sizeof(build_run_root),
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
    fprintf(report, "Mode: build all profile kernels first, then run headless QEMU tests with live serial output displayed\n");
    fprintf(report, "Build root: %s\n", build_run_root);
    fprintf(report, "Output root: %s\n\n", output_run_root);
    fclose(report);

    OrynLogStep("Building every PIC/APIC/APIC2/HPET matrix kernel first.");
    OrynLogKeyValue("Matrix run", run_id);
    OrynLogKeyValue("Matrix build root", build_run_root);
    OrynLogKeyValue("Matrix output root", output_run_root);
    AppendMatrixSection(matrix_report_path, "Build phase:");

    for (unsigned int index = 0; index < gProfileCount; ++index)
    {
        OrynProject profile_project;
        int ok = 1;

        ConfigureProfileProject(&profile_project, &base_project, &gProfiles[index],
            build_run_root, output_run_root);
        LogProfileSettings(&profile_project, &gProfiles[index], index);

        if (!OrynBuildKernel(&profile_project))
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
            OrynLogOk("Matrix profile built and kept in its own folder.");
        }
        else
        {
            build_failed += 1;
            OrynLogFail("Matrix profile build failed. Its build/output folder was kept for diagnostics.");
        }
    }

    OrynLogStep("Testing built matrix kernels in headless QEMU.");
    OrynLogInfo("Each headless kernel requests QEMU debug-exit after its proof tasks complete.");
    OrynLogInfo("Each headless QEMU test displays the kernel serial stream live in this terminal.");
    AppendMatrixSection(matrix_report_path, "Test phase:");

    for (unsigned int index = 0; index < gProfileCount; ++index)
    {
        OrynProject profile_project;

        ConfigureProfileProject(&profile_project, &base_project, &gProfiles[index],
            build_run_root, output_run_root);

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
            OrynLogOk("Matrix profile passed its headless QEMU test and exited QEMU.");
        }
        else
        {
            test_failed += 1;
            OrynLogFail("Matrix profile QEMU test failed. Its kernel, image, debug log, and boot report were kept.");
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
