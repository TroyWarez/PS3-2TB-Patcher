#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// --- Win32 API Wrapper to silently run scetool.exe ---
bool RunProcessSilent(const string& commandLine) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Create a mutable copy of the command line string (required by CreateProcessA)
    char cmd[512];
    strcpy_s(cmd, commandLine.c_str());

    // Spawn the process hidden
    if (CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE); // Wait for scetool to finish
        DWORD exitCode;
        GetExitCodeProcess(pi.hProcess, &exitCode);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return (exitCode == 0);
    }
    return false;
}

// --- Fatal Error Handler ---
void FatalError(const string& message) {
    cout << "\n=======================================================" << endl;
    cout << "[!] FATAL ERROR: " << message << endl;
    cout << "[!] The exact byte pattern or string could not be located." << endl;
    cout << "[!] Reason 1: The firmware version offsets have shifted." << endl;
    cout << "[!] Reason 2: The file has already been patched." << endl;
    cout << "[!] ABORTING to prevent a console brick." << endl;
    cout << "=======================================================\n" << endl;

    // Clean up any decrypted leftover files before aborting
    DeleteFile(L"lv2_kernel.elf");
    DeleteFile(L"emer_init.elf");

    system("pause");
    exit(1);
}

// --- Binary Pattern Scanner and Patcher ---
bool PatchBinary(const string& filename, const vector<uint8_t>& pattern, const vector<uint8_t>& patch, int patchOffset = 0) {
    // 1. Read the file into memory
    ifstream file(filename, ios::binary | ios::ate);
    if (!file.is_open()) return false;

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);
    vector<uint8_t> buffer(size);
    if (!file.read((char*)buffer.data(), size)) return false;
    file.close();

    // 2. Scan for the hex pattern
    auto it = search(buffer.begin(), buffer.end(), pattern.begin(), pattern.end());
    if (it == buffer.end()) {
        return false; // Let the FatalError handler catch this
    }

    // 3. Calculate exact memory location
    size_t foundOffset = distance(buffer.begin(), it);
    cout << "[+] Pattern found at offset: 0x" << hex << foundOffset << dec << endl;

    // 4. Inject the Patch (e.g., NOP 60 00 00 00)
    for (size_t i = 0; i < patch.size(); ++i) {
        buffer[foundOffset + patchOffset + i] = patch[i];
    }

    // 5. Write the patched file back to disk
    ofstream outfile(filename, ios::binary | ios::trunc);
    if (!outfile.is_open()) return false;
    outfile.write((char*)buffer.data(), buffer.size());
    outfile.close();

    cout << "[+] Successfully patched " << filename << "!" << endl;
    return true;
}

int main() {
    cout << "========================================" << endl;
    cout << "  PS3 2TB+ Patcher v1.3 (Safe Mode)" << endl;
    cout << "========================================" << endl;

    vector<uint8_t> nopKPatch = { 0x60, 0x00, 0x00, 0x00 };

    vector<uint8_t> GatePatch = { 0x38, 0x00, 0x10, 0x00 }; // 16 TB gate increase

    vector<uint8_t> FragmentPatch = { 0x38, 0x00, 0x40, 0x00 };

    vector<uint8_t> DensityPatch = { 0x38, 0x00, 0x80, 0x00 };

    /*
       NOTE: You must verify these byte patterns in IDA Pro for your specific firmware.
       These are example representations of the assembly described in the documentation.
    */

    // Pattern 1: LV2 Kernel Truncation (clrldi r28, r28, 32 -> add r0, r0, r9)
    vector<uint8_t> lv2_pattern = { 0x78, 0x0B, 0x00, 0x20,  // clrldi r0, r11, 32
                                    0x7B, 0x9C, 0x00, 0x20,  // clrldi r28, r28, 32 (Target)
                                    0x7C, 0x00, 0x4A, 0x14 }; // add r0, r0, r9

    // Pattern 2: emer_init 1.5TB Gate
    vector<uint8_t> emer_pattern_gate = { 0x40, 0x9E, 0x01, 0x50,
                                          0x38, 0x00, 0x01, 0x80,
                                          0xE8, 0x81, 0x00, 0x78 };

	// Pattern 3: emer_init fragement size
    vector<uint8_t> emer_pattern_fragment = { 0x90, 0x04, 0x00, 0x00,
                                      0x38, 0x00, 0x20, 0x00,
                                      0x91, 0x06, 0x00, 0x00 };

    // Pattern 4: emer_init inode density size
    vector<uint8_t> emer_pattern_density = { 0xF8, 0x01, 0x01, 0x10,
                                      0x38, 0x00, 0x10, 0x00,
                                      0xFA, 0x01, 0x00, 0x80 };



    // --- STEP 1: DECRYPT ---
    cout << "\n[*] Decrypting files via scetool..." << endl;
    if (!RunProcessSilent("scetool.exe -d lv2_kernel.self lv2_kernel.elf")) FatalError("Failed to decrypt LV2 Kernel! Is scetool missing?");
    if (!RunProcessSilent("scetool.exe -d emer_init.self emer_init.elf")) FatalError("Failed to decrypt emer_init!");


    cout << "[*] Scanning and Patching LV2 Kernel (2TB+ Truncation)..." << endl;
    if (!PatchBinary("lv2_kernel.elf", lv2_pattern, nopKPatch, 4)) {
        FatalError("LV2 Kernel 32-bit truncation pattern missing.");
    }

    cout << "[*] Scanning and Patching emer_init (1.5TB Gate)..." << endl;
    if (!PatchBinary("emer_init.elf", emer_pattern_gate, GatePatch, 4)) {
        FatalError("emer_init 1.5TB gate pattern missing.");
    }

    cout << "[*] Scanning and Patching emer_init (Block Size)..." << endl;
    if (!PatchBinary("emer_init.elf", emer_pattern_density, DensityPatch, 4)) {
        FatalError("emer_init block size pattern missing.");
    }

    cout << "[*] Scanning and Patching emer_init (Fragment Size)..." << endl;
    if (!PatchBinary("emer_init.elf", emer_pattern_fragment, FragmentPatch, 4)) {
        FatalError("emer_init fragment size pattern missing.");
    }

    // --- STEP 3: RE-ENCRYPT ---
    cout << "\n[*] Re-encrypting patched files..." << endl;


    if (!RunProcessSilent("scetool.exe -0 SELF -t lv2_kernel.self -1 TRUE -e lv2_kernel.elf lv2_kernel_patched.self")) {
        FatalError("Failed to re-encrypt lv2_kernel.self!");
    }

    if (!RunProcessSilent("scetool.exe -0 SELF -t emer_init.self -1 TRUE -e emer_init.elf emer_init_patched.self")) {
        FatalError("Failed to re-encrypt emer_init.self!");
    }


    // --- STEP 4: CLEANUP ---
    DeleteFile(L"lv2_kernel.elf");
    DeleteFile(L"emer_init.elf");

    cout << "\n[+] SUCCESS! All patched files have been created successfully." << endl;
    cout << "[+] You may now create your NEW-PS3UPDAT.PUP using PUAD GUI and the repacking option by replacing the original files." << endl;
    system("pause");
    return 0;
}