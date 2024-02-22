#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    bool printGPUDetails, printProcDetails;
} CLIFlags;

CLIFlags newCLIFlags(bool printGPUDetails, bool printProcDetails) {
    CLIFlags flags;
    flags.printGPUDetails = printGPUDetails;
    flags.printProcDetails = printProcDetails;
    return flags;
}

void printHelp(char* programName) {
    fprintf(stderr,
            "Usage:\n"
            " %s [options]\n"
            "Displays a simplified version of the output from the `nvidia-smi` command.\n"
            "When no options are provided, it displays both GPU details and process information.\n"
            "\n"
            "Options:\n"
            " -g, --gpus     display GPU details\n"
            " -p, --procs    display process details\n"
            " -h, --help     display this help and exit\n\n",
            programName);
}

CLIFlags handleCLIFlags(int argc, char* argv[]) {
    bool printGPUDetailsPassed = false;
    bool printProcDetailsPassed = false;
    for (int i = 1; i < argc; ++i) {
        // Check if the argument is for GPU details
        if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "--gpus")) {
            if (printGPUDetailsPassed) {
                fprintf(stderr, "ERROR: -g/--gpus argument passed multiple times\n");
                exit(2);
            }
            printGPUDetailsPassed = true;
        }
        // Check if the argument is for process details
        else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--procs")) {
            if (printProcDetailsPassed) {
                fprintf(stderr, "ERROR: -p/--procs argument passed multiple times\n");
                exit(2);
            }
            printProcDetailsPassed = true;
        }
        // Check if the help flag is passed
        else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            printHelp(argv[0]);
            exit(0);
        }
        // Handle unrecognized arguments
        else {
            printHelp(argv[0]);
            fprintf(stderr, "ERROR: Unrecognized argument: %s\n", argv[i]);
            exit(2);
        }
    }
    bool noFlagsPassed = !(printGPUDetailsPassed || printProcDetailsPassed);

    return newCLIFlags(printGPUDetailsPassed || noFlagsPassed,
                       printProcDetailsPassed || noFlagsPassed);
}

int main(int argc, char* argv[]) {
    FILE* nvidiaSMIStream;
    char buffer0[4096];

    // get run flags from CLI
    CLIFlags flags = handleCLIFlags(argc, argv);

    // Execute the nvidia-smi command and open a stream to read its output
    nvidiaSMIStream = popen("nvidia-smi", "r");
    if (nvidiaSMIStream == NULL) {
        fprintf(stderr, "ERROR: Failed to run command\n");
        exit(1);
    }

    // consume but do not print the time and date
    (void) fgets(buffer0, sizeof(buffer0), nvidiaSMIStream);

    // Read the next three lines of the command's output and print each if not NULL
    // +---------------------------------------------------------------------------------------+
    // | NVIDIA-SMI 535.154.05             Driver Version: 535.154.05   CUDA Version: 12.2     |
    // |-----------------------------------------+----------------------+----------------------+
    for (int i = 0; i < 3; ++i) {
        if (fgets(buffer0, sizeof(buffer0), nvidiaSMIStream) != NULL) {
            if (flags.printGPUDetails) printf("%s", buffer0);
        }
    }

    // Loops once per GPU on system
    while (fgets(buffer0, sizeof(buffer0), nvidiaSMIStream) != NULL) {
        // if the line starts with a space that means there are no more gpus to be processed
        if (buffer0[0] == ' ') { break; }

        if (flags.printGPUDetails) printf("%.*s", 6, buffer0); // prints "| GPU "

        if (fgets(buffer0, sizeof(buffer0), nvidiaSMIStream) != NULL) {
            // prints " Fan  Temp "
            if (flags.printGPUDetails) printf("%.*s", 11, &buffer0[1]);
            // prints " Perf "
            if (flags.printGPUDetails) printf("%.*s", 6, &buffer0[13]);
            // prints "      Pwr:Usage/Cap |         Memory-Usage | GPU-Util  Compute M. |"
            if (flags.printGPUDetails) printf("%s", &buffer0[23]);
        }

        (void) fgets(buffer0, sizeof(buffer0), nvidiaSMIStream); // skip blank line
        if (fgets(buffer0, sizeof(buffer0), nvidiaSMIStream) != NULL) {
            if (flags.printGPUDetails)
                printf("%s", buffer0); // prints block seperator "...-----..."
        }
    }
    // since the while loop consumes one line before it exits there is no need to handle the blank
    // line following the gpu table

    // if the gpu details were not printed out print the headder for the process table
    if (fgets(buffer0, sizeof(buffer0), nvidiaSMIStream) != NULL) {
        if (!flags.printGPUDetails && flags.printProcDetails) printf("%s", buffer0);
    }

    // dump the line that just says processes
    // "| Processes:                                                                            |"
    (void) fgets(buffer0, sizeof(buffer0), nvidiaSMIStream);

    // print the process table column headders
    // "|  GPU   GI   CI        PID   Type   Process name                            GPU Memory |"
    if (fgets(buffer0, sizeof(buffer0), nvidiaSMIStream) != NULL) {
        if (flags.printProcDetails) printf("%s", buffer0);
    }

    // dump the line that has overflow text from the headders
    // "|        ID   ID                                                             Usage      |"
    (void) fgets(buffer0, sizeof(buffer0), nvidiaSMIStream);

    // print out the rest of the process table
    while (fgets(buffer0, sizeof(buffer0), nvidiaSMIStream) != NULL) {
        if (flags.printProcDetails) printf("%s", buffer0);
    }

    // Close the stream
    pclose(nvidiaSMIStream);

    return 0;
}
