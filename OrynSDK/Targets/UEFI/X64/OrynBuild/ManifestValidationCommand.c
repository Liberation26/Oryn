#include "TargetBuildInternal.h"

int OrynCommandManifestValidate(const char* executable_path, const char* project_file)
{
    OrynProject project;
    if (!OrynLoadProject(executable_path, project_file, &project))
    {
        return 1;
    }

    OrynLogKeyValue("Project", project.name);
    OrynLogKeyValue("Target", project.target);
    OrynLogKeyValue("Toolchain", project.toolchain);
    OrynMakeDirectoryRecursive(project.object_dir);
    ResetBuildPlanDiagnostics(&project);
    return ValidateKernelModuleManifests(&project) ? 0 : 1;
}
