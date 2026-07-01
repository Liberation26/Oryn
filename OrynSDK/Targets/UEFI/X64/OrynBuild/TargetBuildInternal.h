#ifndef ORYN_TARGET_BUILD_INTERNAL_H
#define ORYN_TARGET_BUILD_INTERNAL_H

#include "OrynBuild.h"
#include "CommandsSupport.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
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

typedef struct OrynBuildObjectStats
{
    int SourceCount;
    int CompiledCount;
    int ReusedCount;
    int StaleRemovedCount;
} OrynBuildObjectStats;

typedef struct OrynBootReportFacts
{
    int loader_started;
    int kernel_loaded;
    int entry_printed;
    int virtual_map_prepared;
    int virtual_map_active;
    int boot_services_exited;
    int bootinfo_created;
    int boot_config_prepared;
    int memory_map_requested;
    int bootinfo_memory_map;
    int kernel_jump;
    int kernel_entered;
    int serial_ok;
    int bootinfo_received;
    int kernel_boot_config;
    int kernel_command_line;
    int gdt_installing;
    int gdt_installed;
    int gdt_entries;
    int tss_loaded;
    int idt_installing;
    int idt_installed;
    int idt_entries;
    int interrupt_dispatcher;
    int interrupt_handlers;
    int interrupt_controlled;
    int cpu_exception;
    int cpu_local_apic;
    int cpu_apic2;
    int pic_initialized;
    int pic_remapped;
    int pic_masked;
    int apic_available;
    int apic2_enabled;
    int local_apic_enabled;
    int apic_timer_probe;
    int hpet_rsdp;
    int hpet_checksum;
    int hpet_table;
    int hpet_enabled;
    int hpet_counter;
    int pic_irq0;
    int pic_irq0_count;
    int pic_eoi;
    int apic_timer_interrupt;
    int apic_irq_counters;
    int apic_eoi;
    int interrupt_chain;
    int syscall_core;
    int syscall_packet;
    int syscall_get;
    int syscall_set;
    int syscall_event;
    int syscall_unknown;
    int linux_translator;
    int ms_translator;
    int linux_vector;
    int ms_vector;
    int unknown_linux;
    int unknown_ms;
    int syscall_three;
    int platform_syscalls;
    int syscall_counts;
    int pci_started;
    int pci_rsdp;
    int pci_checksum;
    int pci_mcfg;
    int pci_ecam;
    int pci_config;
    int pci_scan;
    int pci_devices;
    int pci_class;
    int pci_complete;
    int pci_english;
    int smp_started;
    int smp_started_early;
    int smp_rsdp;
    int smp_checksum;
    int smp_madt;
    int smp_cached;
    int smp_topology;
    int smp_early_stage;
    int smp_trampoline;
    int smp_cr3;
    int smp_ipi;
    int smp_init_ipi;
    int smp_startup_ipi;
    int smp_aps_started;
    int smp_early_complete;
    int smp_complete;
    int qemu_debug_colour;
    int kernel_console;
    int screen_scrollback;
    int screen_coloured_cells;
    int screen_scroll_lines;
    int screen_page_scroll;
    int screen_bottom;
    int screen_stable_proof;
    int screen_scrolling;
    int screen_back_buffer;
    int screen_renders_back;
    int screen_present;
    int screen_atomic_present;
    int screen_deferred_flip;
    int screen_line_flip;
    int screen_dirty_line;
    int screen_fast_scroll;
    int screen_refresh_optimized;
    int screen_line_buffered;
    int screen_double_buffer;
    int physical_capacity;
    int keyboard_initialized;
    int keyboard_irq1;
    int keyboard_interrupts;
    int keyboard_pic_unmasked;
    int keyboard_decoder;
    int keyboard_make_break;
    int keyboard_release_stop;
    int keyboard_line;
    int keyboard_page;
    int keyboard_stops_on_release;
    int interactive_interrupts;
    const char* report_display_mode;
    int interactive_display;
    unsigned int headless_timeout_seconds;
    int qemu_timeout;
    int qemu_no_output_timeout;
    int qemu_preboot_failure;
    int interactive_hold;
    int qemu_completion_ok;
    int want_pic;
    int want_apic;
    int want_apic2;
    int want_local_apic;
    int want_hpet;
    unsigned int wanted_cpu_count;
    int want_smp;
    int pic_skipped;
    int apic_skipped;
    int apic2_skipped;
    int hpet_skipped;
    int smp_skipped;
    int cpu_local_apic_ok;
    int pic_ok;
    int apic_ok;
    int apic2_ok;
    int hpet_ok;
    int interrupt_chain_ok;
    int smp_ok;
    int virtual_memory_started;
    int virtual_memory_required_mapped;
    int virtual_memory_switching_cr3;
    int virtual_memory_switched_cr3;
    int virtual_memory_active;
    int system_halted;
    int debug_exit;
    int qemu_exit_or_hold;
    int boot_pass;
} OrynBootReportFacts;

extern const OrynVmMatrixProfile gProfiles[];
extern const unsigned int gProfileCount;

int OrynBootInfoTestAll(const char* executable_path, const OrynProject* project, const char* project_file);
int OrynBootInfoQuestionnaire(const char* executable_path, const char* project_file, const OrynProject* project);
int OrynCommandBootInfo(const char* executable_path, const char* project_file, int argument_count, char** arguments);
void Trim(char* text);
int IsNumberText(const char* text);
int ReadSelectedVariantNumber(const char* selected_path);
void OrynResolveBootInfoSelection(OrynProject* project);
int NextVariantNumber(const char* root);
unsigned long long BootInfoSelectionMask( int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data);
int ReadHeaderDefineInt(const char* header_path, const char* define_name, int* value);
int ReadVariantSelection(const char* root, const char* folder_name, int* kernel_range, int* memory_map, int* framebuffer, int* rsdp, int* firmware_data);
int FindExistingVariantNumber(const char* root, int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data);
int AskYesNo(const char* question, int default_yes);
int WriteSelectedNumber(const char* root, int number);
int WriteSelectionHeader(const char* include_dir, int number, int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data);
int WriteVariantNotes(const char* variant_dir, int number, int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data);
const char* YesNoText(int value);
int ParseVariantNumberText(const char* text, int* number);
int CompareNumbers(const void* left, const void* right);
int CollectVariantNumbers(const char* root, int* numbers, int max_numbers);
int LoadVariantByNumber(const char* root, int number, int* kernel_range, int* memory_map, int* framebuffer, int* rsdp, int* firmware_data);
void WriteSelectionNames(FILE* file, int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data);
void PrintSelectionNames(int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data);
void PrintVariantLine(int number, int selected, int kernel_range, int memory_map, int framebuffer, int rsdp, int firmware_data);
int ValidateVariantExists(const OrynProject* project, int number);
int SelectedVariantNumber(const OrynProject* project);
int ShowVariantDetails(const OrynProject* project, int number);
int OrynBootInfoList(const OrynProject* project);
int OrynBootInfoShow(const OrynProject* project, int argument_count, char** arguments);
int OrynBootInfoSelect(const OrynProject* project, int argument_count, char** arguments);
void PrintCompareLine(const char* label, int left, int right);
int OrynBootInfoCompare(const OrynProject* project, int argument_count, char** arguments);
int OrynBootInfoRun(const char* executable_path, const OrynProject* project, const char* project_file, int argument_count, char** arguments);
void WriteVariantTestReportHeader(FILE* file, const OrynProject* project);
void BuildCompileCommand( const OrynProject* project, const char* source_file, const char* object_file, const char* dependency_file, char* command, size_t command_size);
int CompileSourceFile( const OrynProject* project, const char* source_file, char* object_file, size_t object_file_size, int* was_compiled);
void OrynHashHeaderTreeRecursive(unsigned long long* hash, const char* directory);
void OrynHashHeaderTree(unsigned long long* hash, const char* directory);
int HashDependencyToken(unsigned long long* hash, const char* token);
int HashDependencyFile(unsigned long long* hash, const char* dependency_file);
unsigned long long OrynHashBytes(unsigned long long hash, const unsigned char* bytes, size_t count);
unsigned long long OrynHashText(unsigned long long hash, const char* text);
int OrynHashFile(unsigned long long* hash, const char* path);
unsigned long long ComputePathHash(const char* path);
int ReadStoredHash(const char* hash_file, unsigned long long* value);
void WriteStoredHash(const char* hash_file, unsigned long long value);
void LogSelectedBootInfoVariant(const OrynProject* project);
int CollectKernelSources(const OrynProject* project, OrynStringList* sources);
int CompileKernelSources( const OrynProject* project, const OrynStringList* sources, OrynStringList* objects, OrynBuildObjectStats* stats);
void BuildKernelLinkCommand(const OrynProject* project, const OrynStringList* objects, char* command, size_t command_size);
void LogIncrementalSummary(const OrynBuildObjectStats* stats);
int LinkKernelObjects(const OrynProject* project, const OrynStringList* objects);
void LogKernelLayout(const OrynProject* project);
int OrynBuildKernel(const OrynProject* project);
int GenerateKernelModuleManifestTables(const OrynProject* project);
int ValidateLibCFunctionUnitManifests(const OrynProject* project);
void BuildObjectManifestPath(const OrynProject* project, char* output, size_t output_size);
void WriteObjectManifest( const OrynProject* project, const OrynStringList* sources, const OrynStringList* objects, const OrynBuildObjectStats* stats);
void BuildObjectFileName(const OrynProject* project, const char* source_file, char* object_file, size_t object_file_size);
void BuildObjectSidecarPath(char* output, size_t output_size, const char* object_file, const char* extension);
int ProjectBoolEnabledBuild(const char* value, int default_value);
unsigned int ProjectCpuCountBuild(const OrynProject* project);
int ProjectDisplayIsInteractiveBuild(const OrynProject* project);
int IsLegacyProjectSharedSource(const OrynProject* project, const char* source_file);
int AppendSourcesFromDirectory(const OrynProject* project, const char* directory, OrynStringList* sources, const char* label);
void HashCompilerSignature(unsigned long long* hash, const OrynProject* project);
unsigned long long ComputeSourceBuildHash( const OrynProject* project, const char* source_file, const char* dependency_file);
int ObjectListContainsBaseName(const OrynStringList* objects, const char* base_name);
void RemoveSidecarIfPresent(const char* object_path, const char* extension);
int RemoveStaleObjectFiles(const OrynProject* project, const OrynStringList* objects);
int EndsWithBuild(const char* text, const char* suffix);
int PathContainsBuild(const char* path, const char* needle);
int TextEqualsIgnoreCaseBuild(const char* left, const char* right);
void WriteBootReport( const OrynProject* project, const char* report_path, const char* qemu_path, const char* ovmf_windows, const char* ovmf_qemu, const char* disk_windows, const char* debug_log, const char* resolved_cpu, const char* command, const char* debug_text, int exit_code, int command_ok);
void CollectBootReportFacts( const OrynProject* project, const char* debug_text, int exit_code, int command_ok, OrynBootReportFacts* facts);
int TextEqualsIgnoreCaseCommand(const char* left, const char* right);
int ProjectBoolEnabled(const char* value, int default_value);
unsigned int ProjectCpuCount(const OrynProject* project);
const char* OnOffText(int value);
int IsInteractiveDisplayMode(const char* display_mode);
unsigned int ResolveQemuHeadlessTimeoutSeconds(void);
const char* ResolveQemuCpuModel(const char* configured_cpu);
int QemuCpuWasTranslated(const char* configured_cpu, const char* resolved_cpu);
int DebugTextIsEmpty(const char* text);
int IsSafeQemuArgumentValue(const char* value);
void BuildProjectImagePath(const OrynProject* project, char* output, size_t output_size);
int OrynCommandDoctor(const char* executable_path);
int OrynCommandBuild(const char* executable_path, const char* project_file);
int OrynCommandImage(const char* executable_path, const char* project_file);
int OrynCommandRun(const char* executable_path, const char* project_file);
int OrynCommandClean(const char* executable_path, const char* project_file);
void BuildQemuStageProjectName(const OrynProject* project, char* output, size_t output_size);
int OrynRunQemu(const OrynProject* project);
int OrynCleanProject(const OrynProject* project);
void WriteProfileReportLine( FILE* report, const OrynProject* project, const OrynVmMatrixProfile* profile, const char* phase, const char* result);
void AppendProfileReport( const char* matrix_report_path, const OrynProject* project, const OrynVmMatrixProfile* profile, const char* phase, const char* result);
void LogProfileSettings(const OrynProject* project, const OrynVmMatrixProfile* profile, unsigned int index);
void AppendMatrixSection(const char* matrix_report_path, const char* title);
int OrynCommandMatrixInternal( const char* executable_path, const char* project_file, const char* matrix_folder, const char* display_mode, const char* mode_title);
int OrynCommandMatrix(const char* executable_path, const char* project_file);
int OrynCommandMatrixSerial(const char* executable_path, const char* project_file);
int OrynCommandMatrixScreen(const char* executable_path, const char* project_file);
int OrynCommandMatrixAll(const char* executable_path, const char* project_file);
const char* OnOff(int value);
void SetOnOff(char* output, size_t output_size, int value);
int MatrixTextEqualsIgnoreCase(const char* left, const char* right);
int MatrixDisplayIsInteractive(const char* display_mode);
void MakeMatrixRunId(char* output, size_t output_size);
void ResolveUniqueRunRoots( const OrynProject* project, const char* matrix_folder, char* build_run_root, size_t build_run_root_size, char* output_run_root, size_t output_run_root_size, char* run_id, size_t run_id_size);
void ConfigureProfileProject( OrynProject* profile_project, const OrynProject* base_project, const OrynVmMatrixProfile* profile, const char* build_run_root, const char* output_run_root, const char* display_mode);
int SnapshotShouldSkip(const char* name);
int CopyDirectorySnapshotRecursive(const char* source, const char* target);
int CopySnapshotPath(const char* snapshot_root, const char* relative_target, const char* source);
void BuildSourceSnapshotRoot(const OrynProject* project, char* output, size_t output_size);
void BuildProfileManifestPath(const OrynProject* project, char* output, size_t output_size);
int WriteProfileBuildManifest(const OrynProject* project, const OrynVmMatrixProfile* profile);
int SnapshotProfileSources(const OrynProject* project, const OrynVmMatrixProfile* profile);


#define ORYN_MAX_BUILD_MODULES 96
#define ORYN_MAX_MODULE_REQUIRES 8

typedef struct OrynBuildModule
{
    char Name[128];
    char SourceRoot[ORYN_MAX_PATH];
    char ArchivePath[ORYN_MAX_PATH];
    char Requires[ORYN_MAX_MODULE_REQUIRES][128];
    int RequireCount;
    int Recursive;
    int Present;
    int Visiting;
    int Resolved;
    OrynStringList* Sources;
    OrynStringList* Objects;
} OrynBuildModule;

typedef struct OrynBuildArchivePlan
{
    OrynBuildModule Modules[ORYN_MAX_BUILD_MODULES];
    int ModuleCount;
    int ResolvedOrder[ORYN_MAX_BUILD_MODULES];
    int ResolvedCount;
} OrynBuildArchivePlan;

void BuildArchiveDirectory(const OrynProject* project, char* output, size_t output_size);
void BuildArchivePath(const OrynProject* project, const char* module_name, char* output, size_t output_size);
int CollectCFilesFromDirectoryMode(const OrynProject* project, const char* module_name, const char* directory, int recursive, OrynStringList* list);
int AddBuildModule(OrynBuildArchivePlan* plan, const OrynProject* project, const char* name, const char* source_root, int recursive, const char* requires);
int BuildKernelArchivePlan(const OrynProject* project, OrynBuildArchivePlan* plan);
int ResolveKernelArchivePlan(const OrynProject* project, OrynBuildArchivePlan* plan);
int CompileKernelArchivePlan(const OrynProject* project, OrynBuildArchivePlan* plan, OrynStringList* all_sources, OrynStringList* all_objects, OrynBuildObjectStats* stats);
int ArchiveKernelModule(const OrynProject* project, OrynBuildModule* module);
void WriteArchiveManifest(const OrynProject* project, const OrynBuildArchivePlan* plan, const OrynBuildObjectStats* stats);
void BuildKernelArchiveLinkCommand(const OrynProject* project, const OrynBuildArchivePlan* plan, char* command, size_t command_size);
int LinkKernelArchives(const OrynProject* project, const OrynBuildArchivePlan* plan);

void BuildPlanDiagnosticsPath(const OrynProject* project, char* output, size_t output_size);
void ResetBuildPlanDiagnostics(const OrynProject* project);
void AppendBuildPlanDiagnostic(const OrynProject* project, const char* category, const char* decision, const char* detail);
void LogBuildPlanDecision(const OrynProject* project, const char* decision, const char* detail);
void LogBuildPlanSkip(const OrynProject* project, const char* decision, const char* detail);

#endif
