
#include <capstone/capstone.h>
#include <fstream>
#include <vector>
#include <imgui.h>
#include <string>
#include <sstream>

// ---------- ตัวช่วย ----------
std::vector<uint8_t> LoadFileBytes(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file), {});
}

// ---------- ฟังก์ชันหลัก ----------
void ShowDisasmUI() {
    static char filename[256] = "dump.bin";
    static uint64_t baseAddress = 0x1000;
    static uint64_t sizeLimit = 0x200; // จำนวน byte ที่จะอ่าน

    static std::vector<std::string> asmLines;

    ImGui::InputText("Binary file", filename, sizeof(filename));
    ImGui::InputScalar("Base address", ImGuiDataType_U64, &baseAddress, 0, 0, "%llX");
    ImGui::InputScalar("Size to disasm", ImGuiDataType_U64, &sizeLimit, 0, 0, "%llX");

    if (ImGui::Button("Disassemble")) {
        asmLines.clear();

        std::vector<uint8_t> data = LoadFileBytes(filename);
        if (data.empty()) {
            asmLines.push_back("[Error] File not found or empty.");
            return;
        }

        if (sizeLimit > data.size())
            sizeLimit = data.size();

        cs_handle_t* handle;
        cs_err err = cs_new(CS_ARCH_X86, CS_MODE_64, nullptr, &handle);
        if (err != CS_ERR_OK) {
            asmLines.push_back("[Error] Capstone init failed.");
            return;
        }

        cs_insn_iter_t* iter;
        err = cs_inspect(handle, data.data(), sizeLimit, baseAddress, nullptr, &iter);
        if (err != CS_ERR_OK) {
            asmLines.push_back("[Error] Capstone inspect failed.");
            cs_close(&handle);
            return;
        }

        cs_insn_t insn;
        while (cs_inspect_iter(iter, &insn)) {
            std::stringstream ss;
            ss << "0x" << std::hex << insn.address << ":\t"
               << insn.mnemonic << "\t" << insn.op_str;
            asmLines.push_back(ss.str());
        }

        cs_close(&handle);
    }

    ImGui::Separator();
    ImGui::BeginChild("DisasmOutput", ImVec2(0, 400), true);
    for (const auto& line : asmLines)
        ImGui::TextUnformatted(line.c_str());
    ImGui::EndChild();
}
#include <windows.h>
#include <iostream>
#include <string>
#include <capstone/capstone.h>
#include "Dbghyip.h"   // ตัวอย่าง dbg lib headers
#include "catool.h"    // debug tool lib
#include "ZwCore.h"

void DebugFunction(const std::string& funcName, BYTE address[8], BYTE ptrAddressPang[8] = nullptr) {
    std::cout << "[DBG] Function: " << funcName << std::endl;

    std::cout << "[DBG] address: ";
    for (int i = 0; i < 8; i++) printf("%02X ", address[i]);
    std::cout << std::endl;

    if (ptrAddressPang) {
        std::cout << "[DBG] ptrAddressPang: ";
        for (int i = 0; i < 8; i++) printf("%02X ", ptrAddressPang[i]);
        std::cout << std::endl;
    }

    // ตัวอย่างหยิบข้อมูล memory
    uint8_t memBuf[16];
    SIZE_T bytesRead = 0;
    // สมมุติ address เป็น pointer
    PVOID target = *(PVOID*)address;
    ReadProcessMemory(GetCurrentProcess(), target, memBuf, sizeof(memBuf), &bytesRead);
    if (bytesRead == sizeof(memBuf)) {
        std::cout << "[DBG] First 16 bytes at target: ";
        for (int i = 0; i < sizeof(memBuf); i++) printf("%02X ", memBuf[i]);
        std::cout << std::endl;
    } else {
        std::cout << "[DBG] ReadProcessMemory failed: " << GetLastError() << std::endl;
    }

    // ใช้ Capstone แปลง opcode แรกจาก address
    csh csHandle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &csHandle) == CS_ERR_OK) {
        cs_insn* insn;
        size_t count = cs_disasm(csHandle, (uint8_t*)target, 16, (uint64_t)target, 0, &insn);
        if (count > 0) {
            for (size_t i = 0; i < count; i++) {
                printf("0x%llx:\t%s\t%s\n",
                        insn[i].address, insn[i].mnemonic, insn[i].op_str);
            }
            cs_free(insn, count);
        } else {
            std::cout << "[DBG] Disasm fail\n";
        }
        cs_close(&csHandle);
    }

    // ตรวจสอบ XMM register ด้วย catool หรือ ZwCore
    // (ตัวอย่างสมมุติ)
    FLOAT128 xmm[8];
    if (GetXmmContext(currentThread, xmm, 8)) {
        std::cout << "[DBG] XMM registers:\n";
        for (int i = 0; i < 8; i++) {
            printf(" XMM%d = %016llx%016llx\n", i,
                   xmm[i].HighPart, xmm[i].LowPart);
        }
    }
}

// การเรียกใช้ตัวอย่าง:
void TestDebug() {
    BYTE addr1[8] = {0x10,0x20,0x30,0x40,0x00,0x00,0x00,0x00};
    DebugFunction("Foo", addr1);

    BYTE addr2[8] = {0x50,0x60,0x70,0x80,0x00,0x00,0x00,0x00};
    BYTE ptr2[8]  = {0xAA,0xBB,0xCC,0xDD,0x00,0x00,0x00,0x00};
    DebugFunction("Bar", addr2, ptr2);
}


// Capstone + ASM Visual Helper Class with offset support #include <capstone/capstone.h> #include <cstdint> #include <vector> #include <string> #include <map> #include <iostream>

struct AsmResult { uint64_t address; std::string mnemonic; std::string op_str; size_t size; std::vector<uint8_t> bytes; bool isPointer; int32_t pointerOffset; };

class AsmMemoryAnalyzer { public: std::vector<AsmResult> results;

bool Analyze(const std::vector<uint8_t>& memory, uint64_t baseAddress) {
    results.clear();
    csh handle;
    if (cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK)
        return false;

    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);
    cs_insn* insn;
    size_t count = cs_disasm(handle, memory.data(), memory.size(), baseAddress, 0, &insn);
    if (count > 0) {
        for (size_t i = 0; i < count; i++) {
            AsmResult r;
            r.address = insn[i].address;
            r.mnemonic = insn[i].mnemonic;
            r.op_str = insn[i].op_str;
            r.size = insn[i].size;
            r.bytes.assign(insn[i].bytes, insn[i].bytes + insn[i].size);
            r.isPointer = false;
            r.pointerOffset = 0;

            // Attempt pointer offset extraction
            if (insn[i].detail && insn[i].detail->x86.op_count > 0) {
                for (int j = 0; j < insn[i].detail->x86.op_count; j++) {
                    cs_x86_op op = insn[i].detail->x86.operands[j];
                    if (op.type == X86_OP_MEM) {
                        r.isPointer = true;
                        r.pointerOffset = op.mem.disp;
                    }
                }
            }
            results.push_back(r);
        }
        cs_free(insn, count);
    }
    cs_close(&handle);
    return true;
}

// Get raw byte or value at address (from vector)
uint8_t GetByteAt(uint64_t targetAddress, uint64_t base, const std::vector<uint8_t>& mem) const {
    size_t offset = targetAddress - base;
    if (offset >= mem.size()) return 0xFF;
    return mem[offset];
}

void PrintWithColor() const {
    for (const auto& r : results) {
        std::string hex;
        for (auto b : r.bytes) {
            char tmp[8];
            sprintf(tmp, "%02X ", b);
            hex += tmp;
        }

        if (r.isPointer)
            std::cout << "[PTR] ";
        else
            std::cout << "      ";

        printf("0x%llx: %-8s %-20s | %s\n",
            r.address,
            r.mnemonic.c_str(),

