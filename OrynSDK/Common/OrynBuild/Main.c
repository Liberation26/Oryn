#include "OrynBuild.h"
#include <stdio.h>
#include <string.h>

static void PrintUsage(void)
{
    printf("Oryn WSL SDK %s\n", ORYN_VERSION);
    printf("Usage:\n");
    printf("  oryn doctor\n");
    printf("  oryn version\n");
    printf("  oryn build <Project.oryn>\n");
    printf("  oryn image <Project.oryn>\n");
    printf("  oryn run <Project.oryn>\n");
    printf("  oryn matrix <Project.oryn>\n");
    printf("  oryn clean <Project.oryn>\n");
    printf("  oryn bootinfo <Project.oryn> [list|show [n]|select n|compare a b|run n|test-all]\n");
}

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        PrintUsage();
        return 1;
    }

    if (strcmp(argv[1], "version") == 0)
    {
        printf("Oryn WSL SDK %s\n", ORYN_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "doctor") == 0)
    {
        return OrynCommandDoctor(argv[0]);
    }

    if (argc < 3)
    {
        OrynLogFail("Project path is required.");
        PrintUsage();
        return 1;
    }

    if (strcmp(argv[1], "build") == 0)
    {
        return OrynCommandBuild(argv[0], argv[2]);
    }

    if (strcmp(argv[1], "image") == 0)
    {
        return OrynCommandImage(argv[0], argv[2]);
    }

    if (strcmp(argv[1], "run") == 0)
    {
        return OrynCommandRun(argv[0], argv[2]);
    }

    if (strcmp(argv[1], "matrix") == 0)
    {
        return OrynCommandMatrix(argv[0], argv[2]);
    }

    if (strcmp(argv[1], "clean") == 0)
    {
        return OrynCommandClean(argv[0], argv[2]);
    }

    if (strcmp(argv[1], "bootinfo") == 0)
    {
        return OrynCommandBootInfo(argv[0], argv[2], argc - 3, argv + 3);
    }

    OrynLogFail("Unknown command.");
    PrintUsage();
    return 1;
}
