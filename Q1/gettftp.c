#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *prog_name) {
    printf("Usage:\n");
    printf("%s <host> <file>\n", prog_name);
    printf("  <host>: TFTP server address\n");
    printf("  <file>: File to download\n");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *host = argv[1];
    const char *file = argv[2];

    printf("Downloading file '%s' from server '%s'...\n", file, host);
    // Placeholder for TFTP protocol implementation
    return EXIT_SUCCESS;
}
