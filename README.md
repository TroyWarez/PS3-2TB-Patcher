# PS3 2TB+ Internal HDD Auto-Patcher

A C++ automation tool designed to modify core PlayStation 3 firmware files, permanently removing the hardware and software limitations that prevent the use of internal hard drives larger than 1.5 TB. 

By utilizing this tool and repacking a Custom Firmware (CFW) `.PUP`, you can natively install, format, and utilize a 4TB internal SATA hard drive on a PlayStation 3.

## ⚠️ Disclaimer
**USE AT YOUR OWN RISK.** Modifying `lv2_kernel.self` and `emer_init.self` carries a high risk of hard-bricking your console. It is highly recommended to test your generated `.PUP` via the "Boot from USB" Debug option on a DEX console, or ensure you have a hardware flasher (Teensy/Raspberry Pi Pico) installed before flashing CoreOS files to your NAND/NOR. 

*(Note: Testing this firmware in the RPCS3 emulator will result in false positives, as RPCS3 uses High-Level Emulation and bypasses the PS3 kernel and UFS2 block drivers).*

## ✨ Features / Patches Applied

This tool automatically unpacks, hex-patches, and re-encrypts the following bottlenecks:

1. **The 1.5 TB Recovery Gate (`emer_init.self`)**
   * *The Problem:* Safe Mode contains a hardcoded artificial gate that aborts formatting if a drive exceeds 1536 GiB.
   * *The Patch:* Change the `bgt` instruction, allowing the UI to accept up to 16TB drives.
2. **The 8 KB Fragment Size (`emer_init.self`)**
   * *The Problem:* A small fragment size will overrun the 128 kb buffer in the level 2 kernel with large discs
   * *The Patch:* Increase the fragment size to 64 KB
3. **The 16 KB Density File Node Size (`emer_init.self`)**
   * *The Problem:* A small density node size will overrun the 128 kb buffer in the level 2 kernel with large discs
   * *The Patch:* Increase the density size to 16 KB
3. **The 2 TB LV2 Truncation (`lv2_kernel.self`)**
   * *The Problem:* The PS3 hypervisor handles 64-bit math, but the GameOS kernel truncates the region size to 32 bits (`clrldi r28, r28, 32`), blinding the system to anything past 2.2 TB.
   * *The Patch:* NOPs the truncation instruction, leaving the kernel 64-bit clean.
4. **The LV2 Mount Buffer Size (`lv2_kernel.self`)**
   * *The Problem:* The PS3 hypervisor mount buffer is now too small with the current mount buffer size of 128kb,  
   * *The Patch:* The PS3 hypervisor mount buffer is now 1024 kb in size.


## 📋 Prerequisites

* **Windows OS** (The C++ tool utilizes the Win32 API for silent process execution).
* **PUAD GUI v2.0 [WIP]** (To correctly hash and repack the CoreOS `.PUP`).
* A base Custom Firmware (e.g., Evilnat 4.90 or 4.93).

## 🚀 Usage Instructions
### Setup
1. Copy all files from the release folder into the `PUAD GUI\bin` folder
2. Make sure you have the most update to date keys in the `data` folder for your firmware


### Step 1: Extract Base Firmware
1. Open **PAUD GUI Builder**.
2. Click on the ellipses (...) and open your target * `.PUP` file that you extracted earlier in the guide.
3. Then open the folder you wish you use for repacking with the new files and click select a temp folder for your extracted CFW files.
4. Click on the radio button under the mode section select `Unpack`
5. Copy the files in * `update_files/CORE_OS_PACKAGE` named `lv2_kernel.self` and `emer_init.self` to the `PUAD GUI\bin` folder

### Step 2: Run the Patcher
1. Place the extracted `.self` files in the root directory of this tool.
2. Ensure `scetool.exe` and its `data` directory are present.
3. Run `PS3 2TB+ Patcher.exe`.
4. The tool will silently decrypt the files, scan for the required byte patterns, inject the patches, and output `P.self` versions of the files.

### Step 3: Prepare the Patched Files for Injection
Before using PUAD GUI, you must rename the output files back to their original names and place them in a specific folder structure so MFW Builder knows exactly where to inject them.

1. Rename the files:
   * `lv2_kernelP.self` ➔ `lv2_kernel.self`
   * `emer_initP.self` ➔ `emer_init.self`
2. Create a new folder on your desktop called `Custom_PS3_Files`.
3. Inside that folder, create the following exact directory structure and place the files inside:
   * `update_files/CORE_OS_PACKAGE/lv2_kernel.self`
   * `update_files/CORE_OS_PACKAGE/emer_init.self`

### Step 4: Build the Firmware using PAUD GUI

1. Open **PAUD GUI Builder**.
2. Click on the ellipses (...) and open your target * `.PUP` file that you extracted earlier in the guide.
3. Then open the folder you wish you use for repacking with the new files and click select folder.
4. Click on the radio button under the mode section select `Repack`
5. Also select the radio button under the `Firmware` section, select whatever firmware yours happens to be: "FW 4.XX" or  "FW 3.XX"
6. Lastly select the radio button under the `CoreOS` and click on the blue button named `CORE_OS`
7. Now click start
8. Finally replace the files in * `Custom_PS3_Files\update_files\CORE_OS_PACKAGE` with `lv2_kernel.self` and `emer_init.self`

### Step 5: Format and Install
1. Put your newly built `PS3UPDAT.PUP` on a FAT32 USB drive (`PS3/UPDATE/PS3UPDAT.PUP`).
2. Insert your blank 4TB drive into the PS3.
3. Boot the PS3 into **Safe Mode**.
4. Select **Option 6: System Update**.
5. The PS3 will read your patched firmware, bypass the 1.5TB UI gate and boot into the XMB utilizing the full 64-bit geometry!

## 🛑 Troubleshooting: "FATAL ERROR: Pattern Missing"

If the tool aborts and displays a **FATAL ERROR**, this is a built-in safety mechanism. It means the tool scanned the decrypted `.elf` file and could not find the exact assembly hex bytes required to inject the patch. 

The tool aborts automatically to prevent generating a corrupted kernel that would brick your console.

**Why did this happen?**
1. **Firmware Version Mismatch (Offset Shifting):** Every time Sony releases a new firmware (e.g., 4.90 vs 4.91), the C++ compiler injects new timestamps and version strings. This shifts the physical location of the code, and sometimes alters the surrounding registers used by the compiler.
2. **Already Patched:** You may be trying to patch a file that has already been modified.

**How to fix it:**
You must manually verify the new hex pattern for your specific firmware version:
1. Open the decrypted `lv2_kernel.elf` or `emer_init.elf` in a disassembler like **IDA Pro** or **Ghidra**.
2. Search for the assembly instructions documented in the source code.
3. Once found, copy the new Hex Opcode sequence.
4. Open the `PS3 2TB+ Patcher.cpp` source code of this tool, update the `vector<uint8_t>` hex arrays to match your firmware's specific pattern, and recompile the tool.

## 🛠️ Compiling the Source
Open `PS3 2TB+ Patcher.cpp` in Visual Studio or compile via MinGW:
``bash
g++ 'PS3 2TB+ Patcher.cpp' -o PS3_4TB_Patcher.exe -static-libgcc -static-libstdc++``
## Credits
Researcher and finder of the problematic instructions (Original article on the PS3 Dev wiki and mount buffer issue finder)
@sagemono
@TroyWarez
Developer and finder of the missing UFS2 format parameters cylinder-group geometry initial values inside .
