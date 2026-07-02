#include "TargetBuildInternal.h"

void HashCompilerSignature(unsigned long long* hash, const OrynProject* project)
{
    *hash = OrynHashText(*hash, ORYN_VERSION);
    *hash = OrynHashText(*hash, "\ntarget:x86_64-none-elf\n");
    *hash = OrynHashText(*hash, "clang flags:freestanding no-stack no-builtin no-pic no-red-zone\n");
    *hash = OrynHashText(*hash, "\ninclude-common:\n");
    *hash = OrynHashText(*hash, project->sdk_kernel_common_include_dir);
    *hash = OrynHashText(*hash, "\ninclude-target:\n");
    *hash = OrynHashText(*hash, project->sdk_kernel_target_include_dir);
    *hash = OrynHashText(*hash, "\ninclude-project:\n");
    *hash = OrynHashText(*hash, project->include_dir);
    *hash = OrynHashText(*hash, "\ninclude-selection:\n");
    *hash = OrynHashText(*hash, project->selected_kernel_include_dir);
    *hash = OrynHashText(*hash, "\nvmsettings:\n");
    *hash = OrynHashText(*hash, project->run_pic);
    *hash = OrynHashText(*hash, project->run_apic);
    *hash = OrynHashText(*hash, project->run_apic2);
    *hash = OrynHashText(*hash, project->run_hpet);
    *hash = OrynHashText(*hash, project->run_smp);
    *hash = OrynHashText(*hash, project->run_display);
    *hash = OrynHashText(*hash, project->run_storage_interface);
}

unsigned long long ComputeSourceBuildHash(
    const OrynProject* project,
    const char* source_file,
    const char* dependency_file)
{
    unsigned long long hash = 1469598103934665603ULL;
    HashCompilerSignature(&hash, project);
    hash = OrynHashText(hash, "\nsource:\n");
    hash = OrynHashText(hash, source_file);
    hash = OrynHashText(hash, "\n");
    (void)OrynHashFile(&hash, source_file);

    if (dependency_file != 0 && OrynFileExists(dependency_file) && HashDependencyFile(&hash, dependency_file))
    {
        return hash;
    }

    OrynHashHeaderTree(&hash, project->include_dir);
    OrynHashHeaderTree(&hash, project->sdk_kernel_common_include_dir);
    OrynHashHeaderTree(&hash, project->sdk_kernel_target_include_dir);
    OrynHashHeaderTree(&hash, project->selected_kernel_include_dir);
    return hash;
}
