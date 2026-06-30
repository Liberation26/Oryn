#include "TargetBuildInternal.h"
#include <stdio.h>

int CompileSourceFile(
    const OrynProject* project,
    const char* source_file,
    char* object_file,
    size_t object_file_size,
    int* was_compiled)
{
    *was_compiled = 0;
    BuildObjectFileName(project, source_file, object_file, object_file_size);

    char hash_file[ORYN_MAX_PATH];
    char dependency_file[ORYN_MAX_PATH];
    BuildObjectSidecarPath(hash_file, sizeof(hash_file), object_file, ".hash");
    BuildObjectSidecarPath(dependency_file, sizeof(dependency_file), object_file, ".d");

    unsigned long long source_hash = ComputeSourceBuildHash(project, source_file, dependency_file);
    unsigned long long stored_hash = 0ULL;
    if (OrynFileExists(object_file) && ReadStoredHash(hash_file, &stored_hash) && stored_hash == source_hash)
    {
        char message[ORYN_MAX_PATH + 64];
        snprintf(message, sizeof(message), "C object unchanged: %s", source_file);
        OrynLogOk(message);
        return 1;
    }

    char message[ORYN_MAX_PATH + 64];
    snprintf(message, sizeof(message), "Compiling C source to object: %s", source_file);
    OrynLogStep(message);

    char command[ORYN_MAX_PATH * 10];
    BuildCompileCommand(project, source_file, object_file, dependency_file, command, sizeof(command));
    if (!OrynRunCommand(command))
    {
        return 0;
    }

    unsigned long long compiled_hash = ComputeSourceBuildHash(project, source_file, dependency_file);
    WriteStoredHash(hash_file, compiled_hash);
    *was_compiled = 1;
    return 1;
}
