#include <windows.h>
#include <stdio.h>
#include <string.h>

int main() {
    DWORD childPID = GetCurrentProcessId();
    
    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hStdIn == INVALID_HANDLE_VALUE || hStdOut == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[child_1 PID:%lu] Failed to get standard handles\n", childPID);
        return 1;
    }

    printf("[child_1 PID:%lu] Started. Reading from pipe1, writing to file1\n", childPID);

    char buffer[1024];
    DWORD bytesRead;


    while (ReadFile(hStdIn, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';

        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        if (len > 0 && buffer[len - 1] == '\r') {
            buffer[len - 1] = '\0';
            len--;
        }

        char inverted[1024];
        for (size_t i = 0; i < len; i++) {
            inverted[i] = buffer[len - 1 - i];
        }
        inverted[len] = '\0';

        char outBuffer[1026];
        int outLen = sprintf(outBuffer, "%s\n", inverted);

        DWORD bytesWritten;
        if (!WriteFile(hStdOut, outBuffer, outLen, &bytesWritten, NULL)) {
            fprintf(stderr, "[child_1 PID:%lu] Failed to write to output\n", childPID);
            return 1;
        }
    }

    printf("[child_1 PID:%lu] EOF received. Process finished.\n", childPID);
    return 0;
}