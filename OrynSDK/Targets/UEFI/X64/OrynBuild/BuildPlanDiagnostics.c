#include "TargetBuildInternal.h"
#include <stdio.h>
#include <string.h>

void BuildPlanDiagnosticsPath(const OrynProject* project, char* output, size_t output_size)
{
    char plan_dir[ORYN_MAX_PATH];
    OrynJoinPath(plan_dir, sizeof(plan_dir), project->build_dir, "Plan");
    OrynJoinPath(output, output_size, plan_dir, "BuildPlanDiagnostics.txt");
}

void AppendBuildPlanDiagnostic(const OrynProject* project, const char* category, const char* decision, const char* detail)
{
    char plan_dir[ORYN_MAX_PATH];
    char report_path[ORYN_MAX_PATH];
    OrynJoinPath(plan_dir, sizeof(plan_dir), project->build_dir, "Plan");
    OrynMakeDirectoryRecursive(plan_dir);
    BuildPlanDiagnosticsPath(project, report_path, sizeof(report_path));

    FILE* file = fopen(report_path, "ab");
    if (file == 0)
    {
        return;
    }

    fprintf(file, "%s | %s | %s\r\n", category, decision, detail == 0 ? "" : detail);
    fclose(file);
}

void ResetBuildPlanDiagnostics(const OrynProject* project)
{
    char plan_dir[ORYN_MAX_PATH];
    char report_path[ORYN_MAX_PATH];
    OrynJoinPath(plan_dir, sizeof(plan_dir), project->build_dir, "Plan");
    OrynMakeDirectoryRecursive(plan_dir);
    BuildPlanDiagnosticsPath(project, report_path, sizeof(report_path));

    FILE* file = fopen(report_path, "wb");
    if (file == 0)
    {
        OrynLogWarn("Could not reset build-plan diagnostics report.");
        return;
    }

    fprintf(file, "Oryn build-plan diagnostics\r\n");
    fprintf(file, "Version=%s\r\n", ORYN_VERSION);
    fprintf(file, "Project=%s\r\n", project->name);
    fprintf(file, "Target=%s\r\n", project->target);
    fprintf(file, "Toolchain=%s\r\n\r\n", project->toolchain);
    fclose(file);

    OrynLogOk("Build-plan diagnostics report reset.");
}

void LogBuildPlanDecision(const OrynProject* project, const char* decision, const char* detail)
{
    char message[ORYN_MAX_PATH + 160];
    snprintf(message, sizeof(message), "Build-plan decision: %s - %s", decision, detail == 0 ? "" : detail);
    OrynLogInfo(message);
    AppendBuildPlanDiagnostic(project, "DECISION", decision, detail);
}

void LogBuildPlanSkip(const OrynProject* project, const char* decision, const char* detail)
{
    char message[ORYN_MAX_PATH + 160];
    snprintf(message, sizeof(message), "Build-plan skip: %s - %s", decision, detail == 0 ? "" : detail);
    OrynLogWarn(message);
    AppendBuildPlanDiagnostic(project, "SKIP", decision, detail);
}
