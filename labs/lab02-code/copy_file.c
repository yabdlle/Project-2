#include <stdio.h>

#define BUF_SIZE 4096

/*
 * Copy the contents of one file into another file
 *   source_file: Name of the source file to copy from
 *   dest_file: Name of the destination file to copy to
 * The destination file is overwritten if it already exists
 * Returns 0 on success and -1 on error
 */
int copy_file(const char *source_file, const char *dest_file) {
    FILE *f1 = fopen(source_file, "r");
    if (f1 == NULL)
        return -1;

    FILE *f2 = fopen(dest_file, "w");
    if (f2 == NULL)
        return -1;

    char buffer[BUF_SIZE];
    size_t bytes;

    while (fread(buffer, sizeof(char), BUF_SIZE, f1) == 1) {
        fwrite(buffer, 1, bytes, f2);
    }
}
int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <source> <dest>\n", argv[0]);
        return 1;
    }

    // copy_file already prints out any errors
    if (copy_file(argv[1], argv[2]) != 0) {
        return 1;
    }
    return 0;
}
