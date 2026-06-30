#include "TargetBuildInternal.h"
#include <stdio.h>

void BuildObjectManifestPath(const OrynProject* project, char* output, size_t output_size)
{
    OrynJoinPath(output, output_size, project->object_dir, "ObjectManifest.txt");
}

void WriteObjectManifest(
    const OrynProject* project,
    const OrynStringList* sources,
    const OrynStringList* objects,
    const OrynBuildObjectStats* stats)
{
    char manifest_path[ORYN_MAX_PATH];
    BuildObjectManifestPath(project, manifest_path, sizeof(manifest_path));

    FILE* file = fopen(manifest_path, "wb");
    if (file == 0)
    {
        OrynLogWarn("Could not write incremental object manifest.");
        return;
    }

    fprintf(file, "Oryn incremental C-to-object manifest\n");
    fprintf(file, "Version=%s\n", ORYN_VERSION);
    fprintf(file, "Sources=%d\n", stats->SourceCount);
    fprintf(file, "Compiled=%d\n", stats->CompiledCount);
    fprintf(file, "Reused=%d\n", stats->ReusedCount);
    fprintf(file, "StaleRemoved=%d\n\n", stats->StaleRemovedCount);

    for (int index = 0; index < sources->count && index < objects->count; ++index)
    {
        fprintf(file, "Source=%s\n", sources->items[index]);
        fprintf(file, "Object=%s\n\n", objects->items[index]);
    }

    fclose(file);
}
