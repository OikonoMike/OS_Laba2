#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_error(const char* msg) {
    DWORD error = GetLastError();
    fprintf(stderr, "[ERROR] %s (Code: %lu)\n", msg, error);
}

void cleanup(
    HANDLE hReadPipe1, HANDLE hWritePipe1,
    HANDLE hReadPipe2, HANDLE hWritePipe2,
    HANDLE hFile1, HANDLE hFile2,
    PROCESS_INFORMATION* pi1, PROCESS_INFORMATION* pi2,
    BOOL pi1_created, BOOL pi2_created
) {
    if (hReadPipe1) CloseHandle(hReadPipe1);
    if (hWritePipe1) CloseHandle(hWritePipe1);
    if (hReadPipe2) CloseHandle(hReadPipe2);
    if (hWritePipe2) CloseHandle(hWritePipe2);
    if (hFile1 != INVALID_HANDLE_VALUE) CloseHandle(hFile1);
    if (hFile2 != INVALID_HANDLE_VALUE) CloseHandle(hFile2);
    
    if (pi1_created) {
        if (pi1->hProcess) CloseHandle(pi1->hProcess);
        if (pi1->hThread) CloseHandle(pi1->hThread);
    }
    if (pi2_created) {
        if (pi2->hProcess) CloseHandle(pi2->hProcess);
        if (pi2->hThread) CloseHandle(pi2->hThread);
    }
}

int main() {
    DWORD parentPID = GetCurrentProcessId();
    printf("[Parent] PID: %lu started\n", parentPID);

    HANDLE hReadPipe1 = NULL, hWritePipe1 = NULL;
    HANDLE hReadPipe2 = NULL, hWritePipe2 = NULL;
    HANDLE hFile1 = INVALID_HANDLE_VALUE;
    HANDLE hFile2 = INVALID_HANDLE_VALUE;

    PROCESS_INFORMATION pi1, pi2;
    STARTUPINFOA si1, si2;
    SECURITY_ATTRIBUTES sa;
    BOOL pi1_created = FALSE, pi2_created = FALSE;

    ZeroMemory(&pi1, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&pi2, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si1, sizeof(STARTUPINFOA));
    ZeroMemory(&si2, sizeof(STARTUPINFOA));

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe1, &hWritePipe1, &sa, 0)) {
        print_error("Failed to create pipe1");
        return 1;
    }

    if (!CreatePipe(&hReadPipe2, &hWritePipe2, &sa, 0)) {
        print_error("Failed to create pipe2");
        CloseHandle(hReadPipe1);
        CloseHandle(hWritePipe1);
        return 1;
    }

    char fileName1[MAX_PATH], fileName2[MAX_PATH];
    printf("\nEnter output file name for child_1: ");
    if (scanf("%255s", fileName1) != 1) {
        fprintf(stderr, "[ERROR] Failed to read filename1\n");
        cleanup(hReadPipe1, hWritePipe1, hReadPipe2, hWritePipe2, 
                hFile1, hFile2, &pi1, &pi2, pi1_created, pi2_created);
        return 1;
    }
    printf("Enter output file name for child_2: ");
    if (scanf("%255s", fileName2) != 1) {
        fprintf(stderr, "[ERROR] Failed to read filename2\n");
        cleanup(hReadPipe1, hWritePipe1, hReadPipe2, hWritePipe2, 
                hFile1, hFile2, &pi1, &pi2, pi1_created, pi2_created);
        return 1;
    }

    hFile1 = CreateFileA(fileName1, GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile1 == INVALID_HANDLE_VALUE) {
        print_error("Failed to create/open file1");
        cleanup(hReadPipe1, hWritePipe1, hReadPipe2, hWritePipe2, 
                hFile1, hFile2, &pi1, &pi2, pi1_created, pi2_created);
        return 1;
    }

    hFile2 = CreateFileA(fileName2, GENERIC_WRITE, 0, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile2 == INVALID_HANDLE_VALUE) {
        print_error("Failed to create/open file2");
        cleanup(hReadPipe1, hWritePipe1, hReadPipe2, hWritePipe2, 
                hFile1, hFile2, &pi1, &pi2, pi1_created, pi2_created);
        return 1;
    }

    si1.cb = sizeof(STARTUPINFOA);
    si1.dwFlags = STARTF_USESTDHANDLES;
    si1.hStdInput = hReadPipe1;
    si1.hStdOutput = hFile1;
    si1.hStdError = hFile1;

    si2.cb = sizeof(STARTUPINFOA);
    si2.dwFlags = STARTF_USESTDHANDLES;
    si2.hStdInput = hReadPipe2;
    si2.hStdOutput = hFile2;
    si2.hStdError = hFile2;

    if (!CreateProcessA(NULL, "child_1.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si1, &pi1)) {
        print_error("Failed to create child_1 process");
        cleanup(hReadPipe1, hWritePipe1, hReadPipe2, hWritePipe2, 
                hFile1, hFile2, &pi1, &pi2, pi1_created, pi2_created);
        return 1;
    }
    pi1_created = TRUE;
    printf("[Parent] child_1.exe started with PID: %lu\n", pi1.dwProcessId);

    if (!CreateProcessA(NULL, "child_2.exe", NULL, NULL, TRUE, 0, NULL, NULL, &si2, &pi2)) {
        print_error("Failed to create child_2 process");
        CloseHandle(hWritePipe1);
        WaitForSingleObject(pi1.hProcess, INFINITE);
        cleanup(hReadPipe1, NULL, hReadPipe2, hWritePipe2, 
                hFile1, hFile2, &pi1, &pi2, pi1_created, pi2_created);
        return 1;
    }
    pi2_created = TRUE;
    printf("[Parent] child_2.exe started with PID: %lu\n", pi2.dwProcessId);

    CloseHandle(hReadPipe1);
    CloseHandle(hReadPipe2);
    CloseHandle(hFile1);
    CloseHandle(hFile2);
    hReadPipe1 = hReadPipe2 = NULL;
    hFile1 = hFile2 = INVALID_HANDLE_VALUE;

    printf("\n[Parent] Ready to accept input. Type 'exit' to finish.\n");
    char buffer[1024];
    
    while (fgets(buffer, sizeof(buffer), stdin)) {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0';
            len--;
        }
        if (len > 0 && buffer[len - 1] == '\r') {
            buffer[len - 1] = '\0';
            len--;
        }

        if (strcmp(buffer, "exit") == 0) {
            printf("[Parent] Exit command received. Finishing...\n");
            break;
        }

        char sendBuffer[1026];
        int sendLen = snprintf(sendBuffer, sizeof(sendBuffer), "%s\n", buffer);
        if (sendLen < 0 || sendLen >= sizeof(sendBuffer)) {
            print_error("String too long");
            continue;
        }

        DWORD bytesWritten;
        if (len > 10) {
            if (!WriteFile(hWritePipe2, sendBuffer, sendLen, &bytesWritten, NULL)) {
                print_error("Failed to write to pipe2");
                break;
            }
            printf("[Parent] Sent to child_2 (len=%lu): %s\n", (unsigned long)len, buffer);
        } else {
            if (!WriteFile(hWritePipe1, sendBuffer, sendLen, &bytesWritten, NULL)) {
                print_error("Failed to write to pipe1");
                break;
            }
            printf("[Parent] Sent to child_1 (len=%lu): %s\n", (unsigned long)len, buffer);
        }
    }

    if (hWritePipe1) CloseHandle(hWritePipe1);
    if (hWritePipe2) CloseHandle(hWritePipe2);
    hWritePipe1 = hWritePipe2 = NULL;

    printf("[Parent] Waiting for child processes to finish...\n");
    WaitForSingleObject(pi1.hProcess, INFINITE);
    WaitForSingleObject(pi2.hProcess, INFINITE);

    DWORD exitCode1, exitCode2;
    GetExitCodeProcess(pi1.hProcess, &exitCode1);
    GetExitCodeProcess(pi2.hProcess, &exitCode2);
    printf("[Parent] child_1 exited with code: %lu\n", exitCode1);
    printf("[Parent] child_2 exited with code: %lu\n", exitCode2);

    cleanup(NULL, NULL, NULL, NULL, INVALID_HANDLE_VALUE, INVALID_HANDLE_VALUE, 
            &pi1, &pi2, pi1_created, pi2_created);

    printf("[Parent] All resources cleaned up. Parent process (PID: %lu) finished.\n", parentPID);
    return 0;
}