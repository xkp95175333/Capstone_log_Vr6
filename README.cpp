
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

