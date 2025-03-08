#include <stdio.h>
#include <stdlib.h>

/*
 * Read the last integers from a binary file
 *   'num_ints': The number of integers to read
 *   'file_name': The name of the file to read from
 * Returns 0 on success and -1 on error
 */
int read_last_ints(const char *file_name, int num_ints) {
    // TODO Not yet implemented

    FILE *f1 = fopen(file_name, "rb");
    if (f1 == NULL) return -1;

    fseek(f1, sizeof(int) * -num_ints, SEEK_END);
    int buffer[num_ints];
    fread(buffer, sizeof(int), num_ints, f1);
    fclose(f1);

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <file_name> <num_ints>\n", argv[0]);
        return 1;
    }

    const char *file_name = argv[1];
    int num_ints = atoi(argv[2]);
    if (read_last_ints(file_name, num_ints) != 0) {
        printf("Failed to read last %d ints from file %s\n", num_ints, file_name);
        return 1;
    }
    return 0;
}
