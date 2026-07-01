#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

#define ORYN_MAX_LIBC_UNIT_MANIFESTS 512

typedef struct OrynLibCUnitManifest
{
    char Category[128];
    char Unit[128];
    char Source[ORYN_MAX_PATH];
    char PublicSymbols[1024];
    char Path[ORYN_MAX_PATH];
} OrynLibCUnitManifest;

static void StripLibCManifestLine(char* text)
{
    size_t length = strlen(text);
    while (length > 0U && (text[length - 1U] == '\n' || text[length - 1U] == '\r' || text[length - 1U] == ' ' || text[length - 1U] == '\t'))
    {
        text[length - 1U] = 0;
        length -= 1U;
    }

    char* start = text;
    while (*start == ' ' || *start == '\t')
    {
        start += 1;
    }

    if (start != text)
    {
        memmove(text, start, strlen(start) + 1U);
    }
}

static int LibCManifestStartsWith(const char* text, const char* prefix)
{
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

static int IsSafeLibCManifestPath(const char* text)
{
    if (text[0] == 0 || text[0] == '/' || strchr(text, '\\') != 0 || strstr(text, "..") != 0)
    {
        return 0;
    }

    return EndsWithBuild(text, ".c");
}

static int IsSafeLibCManifestName(const char* text)
{
    if (text[0] == 0)
    {
        return 0;
    }

    for (const char* cursor = text; *cursor != 0; ++cursor)
    {
        char value = *cursor;
        if (!((value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '_' || value == '-'))
        {
            return 0;
        }
    }

    return 1;
}

static int ParseLibCUnitManifestFile(const char* path, OrynLibCUnitManifest* output)
{
    memset(output, 0, sizeof(*output));
    snprintf(output->Path, sizeof(output->Path), "%s", path);

    FILE* file = fopen(path, "r");
    if (!file)
    {
        return 0;
    }

    char first_line[256];
    if (!fgets(first_line, sizeof(first_line), file))
    {
        fclose(file);
        return 0;
    }
    StripLibCManifestLine(first_line);
    if (strcmp(first_line, "ORYN_LIBC_FUNCTION_UNIT_MANIFEST_V1") != 0)
    {
        fclose(file);
        return 0;
    }

    char line[2048];
    while (fgets(line, sizeof(line), file))
    {
        StripLibCManifestLine(line);
        if (line[0] == 0 || line[0] == '#')
        {
            continue;
        }

        if (LibCManifestStartsWith(line, "Category=")) snprintf(output->Category, sizeof(output->Category), "%s", line + 9);
        else if (LibCManifestStartsWith(line, "Unit=")) snprintf(output->Unit, sizeof(output->Unit), "%s", line + 5);
        else if (LibCManifestStartsWith(line, "Source=")) snprintf(output->Source, sizeof(output->Source), "%s", line + 7);
        else if (LibCManifestStartsWith(line, "PublicSymbols=")) snprintf(output->PublicSymbols, sizeof(output->PublicSymbols), "%s", line + 14);
    }

    fclose(file);
    if (!IsSafeLibCManifestName(output->Category) || !IsSafeLibCManifestName(output->Unit) || !IsSafeLibCManifestPath(output->Source))
    {
        return 0;
    }

    if (output->PublicSymbols[0] == 0 || strcmp(output->PublicSymbols, "-") == 0)
    {
        return 0;
    }

    return 1;
}

static int LoadLibCUnitManifestDirectory(const char* directory, OrynLibCUnitManifest* manifests, int* manifest_count)
{
    DIR* dir = opendir(directory);
    if (!dir)
    {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[ORYN_MAX_PATH];
        OrynJoinPath(child, sizeof(child), directory, entry->d_name);
        struct stat status;
        if (stat(child, &status) != 0)
        {
            closedir(dir);
            return 0;
        }

        if (S_ISDIR(status.st_mode))
        {
            if (!LoadLibCUnitManifestDirectory(child, manifests, manifest_count))
            {
                closedir(dir);
                return 0;
            }
        }
        else if (S_ISREG(status.st_mode) && EndsWithBuild(entry->d_name, ".libcunit"))
        {
            if (*manifest_count >= ORYN_MAX_LIBC_UNIT_MANIFESTS)
            {
                closedir(dir);
                return 0;
            }
            if (!ParseLibCUnitManifestFile(child, &manifests[*manifest_count]))
            {
                closedir(dir);
                return 0;
            }
            *manifest_count += 1;
        }
    }

    closedir(dir);
    return 1;
}

static int CollectLibCSourceRelativePathsRecursive(const char* root, const char* directory, OrynStringList* sources)
{
    DIR* dir = opendir(directory);
    if (!dir)
    {
        return 0;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[ORYN_MAX_PATH];
        OrynJoinPath(child, sizeof(child), directory, entry->d_name);
        struct stat status;
        if (stat(child, &status) != 0)
        {
            closedir(dir);
            return 0;
        }

        if (S_ISDIR(status.st_mode))
        {
            if (!CollectLibCSourceRelativePathsRecursive(root, child, sources))
            {
                closedir(dir);
                return 0;
            }
        }
        else if (S_ISREG(status.st_mode) && EndsWithBuild(entry->d_name, ".c"))
        {
            if (sources->count >= ORYN_MAX_ITEMS)
            {
                closedir(dir);
                return 0;
            }
            const char* relative = child + strlen(root);
            if (*relative == '/')
            {
                relative += 1;
            }
            snprintf(sources->items[sources->count], ORYN_MAX_PATH, "%s", relative);
            sources->count += 1;
        }
    }

    closedir(dir);
    return 1;
}

static int LibCManifestHasSource(const OrynLibCUnitManifest* manifests, int manifest_count, const char* source)
{
    for (int index = 0; index < manifest_count; ++index)
    {
        if (strcmp(manifests[index].Source, source) == 0)
        {
            return 1;
        }
    }

    return 0;
}

static int LibCSourceSeenBefore(const OrynLibCUnitManifest* manifests, int manifest_index)
{
    for (int index = 0; index < manifest_index; ++index)
    {
        if (strcmp(manifests[index].Source, manifests[manifest_index].Source) == 0)
        {
            return 1;
        }
    }

    return 0;
}

int ValidateLibCFunctionUnitManifests(const OrynProject* project)
{
    char source_root[ORYN_MAX_PATH];
    char manifest_root[ORYN_MAX_PATH];
    char plan_dir[ORYN_MAX_PATH];
    char report_path[ORYN_MAX_PATH];
    OrynJoinPath(source_root, sizeof(source_root), project->sdk_root, "Common/OrynLibC/Source");
    OrynJoinPath(manifest_root, sizeof(manifest_root), project->sdk_root, "Common/OrynLibC/FunctionManifests");
    OrynJoinPath(plan_dir, sizeof(plan_dir), project->build_dir, "Plan");
    OrynMakeDirectoryRecursive(plan_dir);
    OrynJoinPath(report_path, sizeof(report_path), plan_dir, "LibCFunctionUnitManifestReport.txt");

    FILE* report = fopen(report_path, "w");
    if (!report)
    {
        OrynLogFail("Could not write LibC function-unit manifest report.");
        return 0;
    }

    fprintf(report, "ORYN_LIBC_FUNCTION_UNIT_MANIFEST_REPORT_V1\n");
    fprintf(report, "SourceRoot=%s\n", source_root);
    fprintf(report, "ManifestRoot=%s\n\n", manifest_root);

    static OrynLibCUnitManifest manifests[ORYN_MAX_LIBC_UNIT_MANIFESTS];
    int manifest_count = 0;
    memset(manifests, 0, sizeof(manifests));
    if (!LoadLibCUnitManifestDirectory(manifest_root, manifests, &manifest_count))
    {
        fprintf(report, "FAIL=Could not load LibC function-unit manifests.\n");
        fclose(report);
        OrynLogFail("LibC function-unit manifest directory could not be loaded.");
        LogBuildPlanSkip(project, "libc-function-manifest", manifest_root);
        return 0;
    }

    static OrynStringList sources;
    sources.count = 0;
    if (!CollectLibCSourceRelativePathsRecursive(source_root, source_root, &sources))
    {
        fprintf(report, "FAIL=Could not collect LibC source files.\n");
        fclose(report);
        OrynLogFail("LibC source files could not be collected for manifest validation.");
        return 0;
    }

    int failures = 0;
    fprintf(report, "ManifestCount=%d\n", manifest_count);
    fprintf(report, "SourceCount=%d\n\n", sources.count);

    for (int index = 0; index < manifest_count; ++index)
    {
        char full_source[ORYN_MAX_PATH];
        OrynJoinPath(full_source, sizeof(full_source), source_root, manifests[index].Source);
        int exists = OrynFileExists(full_source);
        int duplicate = LibCSourceSeenBefore(manifests, index);
        fprintf(report, "Manifest=%s\n", manifests[index].Path);
        fprintf(report, "  Source=%s\n", manifests[index].Source);
        fprintf(report, "  Unit=%s\n", manifests[index].Unit);
        fprintf(report, "  Category=%s\n", manifests[index].Category);
        fprintf(report, "  PublicSymbols=%s\n", manifests[index].PublicSymbols);
        fprintf(report, "  SourceExists=%s\n", exists ? "yes" : "no");
        fprintf(report, "  DuplicateSource=%s\n", duplicate ? "yes" : "no");
        if (!exists || duplicate)
        {
            failures += 1;
        }
    }

    for (int index = 0; index < sources.count; ++index)
    {
        if (!LibCManifestHasSource(manifests, manifest_count, sources.items[index]))
        {
            fprintf(report, "MissingManifestForSource=%s\n", sources.items[index]);
            failures += 1;
        }
    }

    fprintf(report, "\nResult=%s\n", failures == 0 ? "OK" : "FAIL");
    fclose(report);

    char message[192];
    snprintf(message, sizeof(message), "Validated %d LibC function-unit manifest file(s) for %d source unit(s).", manifest_count, sources.count);
    if (failures != 0)
    {
        OrynLogFail("LibC function-unit manifest validation failed.");
        LogBuildPlanSkip(project, "libc-function-manifest", report_path);
        return 0;
    }

    OrynLogOk(message);
    LogBuildPlanDecision(project, "libc-function-manifest", message);
    LogBuildPlanDecision(project, "libc-function-manifest-report", report_path);
    return 1;
}
