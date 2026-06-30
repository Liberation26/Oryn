#ifndef ORYN_BUILD_H
#define ORYN_BUILD_H

#include <stddef.h>

#define ORYN_VERSION "0.5.56"
#define ORYN_MAX_PATH 4096
#define ORYN_MAX_ITEMS 512

typedef struct OrynStringList
{
    char items[ORYN_MAX_ITEMS][ORYN_MAX_PATH];
    int count;
} OrynStringList;

typedef struct OrynProject
{
    char name[128];
    char type[64];
    char target[64];
    char toolchain[64];
    char architecture[64];
    char entry[128];
    char kernel_command_line[256];
    char project_file[ORYN_MAX_PATH];
    char project_root[ORYN_MAX_PATH];
    char sdk_root[ORYN_MAX_PATH];
    char source_dir[ORYN_MAX_PATH];
    char include_dir[ORYN_MAX_PATH];
    char sdk_kernel_common_include_dir[ORYN_MAX_PATH];
    char sdk_kernel_common_source_dir[ORYN_MAX_PATH];
    char sdk_kernel_target_include_dir[ORYN_MAX_PATH];
    char sdk_kernel_target_source_dir[ORYN_MAX_PATH];
    char build_dir[ORYN_MAX_PATH];
    char object_dir[ORYN_MAX_PATH];
    char output_dir[ORYN_MAX_PATH];
    char esp_dir[ORYN_MAX_PATH];
    char ovmf_path[ORYN_MAX_PATH];
    char kernel_variants_root[ORYN_MAX_PATH];
    char selected_kernel_dir[ORYN_MAX_PATH];
    char selected_kernel_include_dir[ORYN_MAX_PATH];
    char run_vm[64];
    char run_display[64];
    char run_memory[64];
    char run_cpu[128];
    char run_smp[32];
    char run_pic[32];
    char run_apic[32];
    char run_apic2[32];
    char run_hpet[32];
    char run_disk_format[32];
    char run_storage_interface[32];
    char run_format_vm[32];
    unsigned long long kernel_physical_base;
    unsigned long long kernel_virtual_base;
    int selected_kernel_number;
} OrynProject;

void OrynLogInfo(const char* message);
void OrynLogStep(const char* message);
void OrynLogOk(const char* message);
void OrynLogWarn(const char* message);
void OrynLogFail(const char* message);
void OrynLogKeyValue(const char* key, const char* value);
void OrynLogCommand(const char* command);

int OrynCommandDoctor(const char* executable_path);
int OrynCommandBuild(const char* executable_path, const char* project_file);
int OrynCommandImage(const char* executable_path, const char* project_file);
int OrynCommandRun(const char* executable_path, const char* project_file);
int OrynCommandClean(const char* executable_path, const char* project_file);
int OrynCommandBootInfo(const char* executable_path, const char* project_file, int argument_count, char** arguments);
int OrynCommandMatrix(const char* executable_path, const char* project_file);
int OrynCommandMatrixSerial(const char* executable_path, const char* project_file);
int OrynCommandMatrixScreen(const char* executable_path, const char* project_file);
int OrynCommandMatrixAll(const char* executable_path, const char* project_file);

int OrynLoadProject(const char* executable_path, const char* project_file, OrynProject* project);
void OrynResolveBootInfoSelection(OrynProject* project);
int OrynBuildKernel(const OrynProject* project);
int OrynBuildImage(const OrynProject* project);
int OrynCreateFat32EspImage(
    const char* boot_efi_path,
    const char* kernel_elf_path,
    const char* font_ttf_path,
    const char* image_path,
    const char* kernel_directory_name,
    const char* kernel_file_name);
int OrynRunQemu(const OrynProject* project);
int OrynCleanProject(const OrynProject* project);

int OrynPathExists(const char* path);
int OrynFileExists(const char* path);
int OrynDirectoryExists(const char* path);
int OrynMakeDirectoryRecursive(const char* path);
int OrynCopyFile(const char* source, const char* target);
int OrynRemoveDirectoryRecursive(const char* path);
void OrynJoinPath(char* output, size_t output_size, const char* left, const char* right);
void OrynGetDirectoryName(char* output, size_t output_size, const char* path);
void OrynGetBaseName(char* output, size_t output_size, const char* path);
void OrynReplaceExtension(char* output, size_t output_size, const char* path, const char* extension);
void OrynNormalizePath(char* path);
void OrynMakeFatDirectoryName(char* output, size_t output_size, const char* input);
void OrynMakeSafeFileBaseName(char* output, size_t output_size, const char* input);
void OrynMakeKernelElfFileName(char* output, size_t output_size, const char* kernel_name);

int OrynRunCommand(const char* command);
int OrynRunCommandCapture(const char* command, char* output, size_t output_size);
int OrynFindProgram(const char* program, char* output, size_t output_size);
int OrynFindOvmf(char* output, size_t output_size);
int OrynFindWindowsQemu(char* output, size_t output_size);
int OrynConvertWslPathToWindows(const char* wsl_path, char* output, size_t output_size);
int OrynConvertWslPathToWindowsQemu(const char* wsl_path, char* output, size_t output_size);
int OrynConvertWindowsPathToWsl(const char* windows_path, char* output, size_t output_size);
int OrynResolveWindowsStageRoot(char* output, size_t output_size);
void OrynMakeStageFilePath(char* output, size_t output_size, const char* stage_root, const char* project_name, const char* suffix);
void OrynShellQuote(char* output, size_t output_size, const char* input);
long long OrynFileModifiedTime(const char* path);
int OrynCollectCFiles(const char* directory, OrynStringList* list);

#endif
