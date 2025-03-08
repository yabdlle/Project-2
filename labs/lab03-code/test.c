#include <stdio.h>
#include <unistd.h>

int main() {
    fork();  // First fork
    fork();  // Second fork
    fork();  // Third fork
    printf("Hello World (Process ID: %d)\n", getpid());
    return 0;
}
