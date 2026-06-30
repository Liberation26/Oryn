#include "TargetBuildInternal.h"
void WriteProfileReportLine(
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

void AppendProfileReport(
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

void LogProfileSettings(const OrynProject* project, const OrynVmMatrixProfile* profile, unsigned int index)
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

void AppendMatrixSection(const char* matrix_report_path, const char* title)
{
    FILE* report = fopen(matrix_report_path, "ab");
    if (report != 0)
    {
        fprintf(report, "%s\n", title);
        fclose(report);
    }
}

int OrynCommandMatrixInternal(
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
