
โอเคเลยครับ 👍
ผมจะทำ ตัวอย่างแยกฟังก์ชัน แต่ละอัน แล้ว printf ให้เห็นผลชัด ๆ
โดยใช้ Capstone (cs_disasm, cs_regs_access, cs_op_index) เพื่อคุณจะลองเทสได้


---

1) หา [rip+...] แล้วดึง offset ออกมา

#include <capstone/capstone.h>
#include <string>
#include <iostream>
#include <cstdint>

int32_t GetRipRelativeOffset(const cs_insn* insn) {
    if (!insn || !insn->op_str) return 0;
    std::string opStr = insn->op_str;
    auto pos = opStr.find("[rip+");
    if (pos == std::string::npos) return 0;

    std::string offsetStr = opStr.substr(pos + 5);
    auto end = offsetStr.find("]");
    if (end != std::string::npos)
        offsetStr = offsetStr.substr(0, end);

    try {
        return std::stoi(offsetStr, nullptr, 16);
    } catch (...) {
        return 0;
    }
}

void ExampleRipOffset() {
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);

    // ตัวอย่าง code มี rip-relative
    uint8_t code[] = { 0x48, 0x8B, 0x05, 0xB8, 0x13, 0x00, 0x00 }; // mov rax, [rip+0x13B8]
    cs_insn* insn;
    size_t count = cs_disasm(handle, code, sizeof(code), 0x140000000, 1, &insn);

    if (count > 0) {
        int32_t rel = GetRipRelativeOffset(&insn[0]);
        std::cout << "Instruction: " << insn[0].mnemonic << " " << insn[0].op_str << "\n";
        std::cout << "Rip-relative offset = 0x" << std::hex << rel << "\n";
        std::cout << "Absolute = 0x" << (insn[0].address + insn[0].size + rel) << "\n";
        cs_free(insn, count);
    }

    cs_close(&handle);
}


---

2) หา Function Start โดยถอยหลัง (Prologue)

#include <immintrin.h>
#include <iostream>
#include <cstdint>

// mock readEx2 (อ่าน memory)
template<typename T>
T readEx2(uintptr_t addr) {
    // ในของจริงคุณจะอ่าน process memory
    // ที่นี่ขอ mock ว่ามี prologue
    T data{};
    uint8_t* p = reinterpret_cast<uint8_t*>(&data);
    p[0] = 0x55; p[1] = 0x48; p[2] = 0x89; p[3] = 0xE5; // push rbp; mov rbp, rsp
    return data;
}

uintptr_t FindFunctionStart(uintptr_t startAddr, size_t maxBack = 0x200) {
    for (size_t offset = 0; offset < maxBack; offset += 16) {
        uintptr_t addr = startAddr - offset;
        __m128i block = readEx2<__m128i>(addr);
        uint8_t* bytes = reinterpret_cast<uint8_t*>(&block);

        if (bytes[0] == 0x55 && bytes[1] == 0x48 && bytes[2] == 0x89 && bytes[3] == 0xE5) {
            return addr;
        }
    }
    return 0;
}

void ExampleFindFunc() {
    uintptr_t target = 0x140012345;
    uintptr_t fnStart = FindFunctionStart(target);
    if (fnStart)
        std::cout << "Function start at: 0x" << std::hex << fnStart << "\n";
    else
        std::cout << "Function start not found\n";
}


---

3) ใช้ Capstone API เพิ่มเติม

(a) ใช้ cs_regs_access → หา register ที่อ่าน/เขียน

void ExampleRegsAccess() {
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    uint8_t code[] = { 0x48, 0x89, 0xD8 }; // mov rax, rbx
    cs_insn* insn;
    size_t count = cs_disasm(handle, code, sizeof(code), 0x1000, 1, &insn);

    if (count > 0) {
        cs_regs regs_read, regs_write;
        uint8_t read_count, write_count;
        if (cs_regs_access(handle, &insn[0], regs_read, &read_count, regs_write, &write_count) == CS_ERR_OK) {
            std::cout << "Instruction: " << insn[0].mnemonic << " " << insn[0].op_str << "\n";
            std::cout << "Read regs: " << (int)read_count << " Write regs: " << (int)write_count << "\n";
        }
        cs_free(insn, count);
    }

    cs_close(&handle);
}

(b) ใช้ cs_op_index → หา operand index ของ type ที่ต้องการ

void ExampleOpIndex() {
    csh handle;
    cs_open(CS_ARCH_X86, CS_MODE_64, &handle);
    cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON);

    uint8_t code[] = { 0x48, 0x89, 0xD8 }; // mov rax, rbx
    cs_insn* insn;
    size_t count = cs_disasm(handle, code, sizeof(code), 0x2000, 1, &insn);

    if (count > 0) {
        int idx = cs_op_index(handle, &insn[0], X86_OP_REG, 0);
        std::cout << "Instruction: " << insn[0].mnemonic << " " << insn[0].op_str << "\n";
        std::cout << "Index of first register operand: " << idx << "\n";
        cs_free(insn, count);
    }

    cs_close(&handle);
}


---

วิธีรันรวม

int main() {
    ExampleRipOffset();
    ExampleFindFunc();
    ExampleRegsAccess();
    ExampleOpIndex();
    return 0;
}


---

✅ แบบนี้คุณจะได้ผลลัพธ์ออกมาเป็น printf/cout ชัดเจน ว่า

offset [rip+...] คือเท่าไหร่

function start เจอตรงไหน

instruction ใช้ register อะไร

operand index อยู่ตำแหน่งไหน



---

คุณอยากให้ผมเขียนเป็น ไฟล์เดียว AsmTest.cpp รวม 4 example นี้ แล้วแยกเป็น main() เลือกทดสอบทีละอันไหมครับ?




__int64 __fastcall sub_140B72010(__int64 a1, __int64 a2)
{
  int v2; // esi
  int v3; // r13d
  __int64 v5; // rax
  unsigned int v6; // r15d
  __int64 v7; // rax
  __int64 v8; // rdx
  __int64 v9; // rax
  unsigned __int64 v10; // r15
  void *v11; // rax
  unsigned __int8 v12; // al
  char v13; // cl
  _WORD *v14; // rax
  _WORD *v15; // rax
  __int32 v16; // edx
  __int32 v17; // ecx
  _WORD *v18; // r14
  unsigned __int64 v19; // rbx
  int v20; // ebx
  __int64 v21; // rax
  __int64 v22; // rax
  _WORD *v23; // rax
  int v24; // edx
  int v25; // ecx
  _WORD *v26; // rdi
  unsigned __int64 v27; // rbx
  int v28; // ebx
  __int64 v29; // rax
  void *v30; // rbx
  void *v31; // rax
  void *v32; // rax
  __int64 v33; // rdx
  __int64 v34; // rcx
  __int64 v35; // r8
  __int64 v36; // r9
  int v37; // eax
  bool v38; // bl
  __int64 v39; // rax
  void *v40; // rdx
  void (__fastcall ***v41)(_QWORD); // rax
  __int64 v42; // rax
  __int64 v43; // rcx
  int v44; // ecx
  int v45; // r15d
  __int64 v46; // rax
  __int64 v47; // rbx
  __int64 v48; // rsi
  int v49; // r14d
  __int64 v50; // rax
  __int64 *v51; // rax
  __int64 *v52; // rdi
  __int64 v53; // rax
  __int64 v54; // rax
  __int64 v55; // rax
  __int64 v56; // rcx
  unsigned __int64 v57; // rax
  _WORD *v58; // rax
  __int64 v59; // rbx
  _WORD *v60; // rdx
  __int64 v61; // rdi
  __int16 v62; // cx
  _WORD *v63; // rax
  int v64; // edx
  int v65; // ecx
  _WORD *v66; // rdi
  unsigned __int64 v67; // rbx
  int v68; // ebx
  __int64 v69; // rax
  __int64 v70; // rax
  int v71; // eax
  char v72; // r14
  int v73; // ebx
  __int64 v74; // rax
  __int64 v75; // rax
  int v76; // eax
  int v77; // ebx
  __int64 v78; // rax
  __int64 v79; // rax
  __int64 v80; // rax
  __int64 v81; // rbx
  __int64 v82; // rdi
  __int64 v83; // rax
  void *v84; // rax
  bool v85; // di
  __int64 v86; // rax
  int v87; // edx
  bool v88; // zf
  __int64 v89; // rbx
  __int64 v90; // rax
  char v91; // r12
  __int64 v92; // rax
  __int64 v93; // rax
  __int64 v94; // rax
  int v95; // edx
  bool v96; // zf
  __int64 v97; // rbx
  __int64 v98; // rax
  char v99; // r14
  __int64 v100; // rax
  __int64 v101; // rax
  _WORD *v102; // rdi
  int v103; // edx
  int v104; // ecx
  unsigned __int64 v105; // rbx
  int v106; // ebx
  __int64 v107; // rax
  char *v108; // rcx
  _WORD *v109; // r8
  signed __int64 v110; // rdx
  __int16 v111; // ax
  __int64 *v112; // rbx
  __int64 v113; // rax
  __int128 *v114; // rcx
  __int64 v115; // rax
  __int64 v116; // rbx
  __int64 v117; // rax
  int v118; // edx
  __int64 v119; // rdi
  int v120; // ecx
  bool v121; // bl
  __int64 v122; // rbx
  __int64 v123; // rax
  __int64 v124; // rax
  __int64 v125; // rax
  __int64 *v126; // rbx
  __int64 v127; // rax
  __int64 v128; // rax
  void *v129; // rax
  __int64 v130; // rax
  __int64 v131; // rax
  int v132; // edx
  bool v133; // bl
  __int64 v134; // rbx
  __int64 v135; // rax
  __int64 v136; // rax
  __int64 v137; // rax
  __int64 v138; // rax
  int v139; // edi
  int v140; // eax
  int v141; // r14d
  __int64 v142; // rbx
  __int64 v143; // rax
  __int64 v144; // rax
  char v145; // bl
  __int64 v146; // rax
  __int64 v147; // rax
  unsigned int v148; // ebx
  unsigned int v149; // edi
  __int64 v150; // rax
  void *v151; // rbx
  __int64 v152; // rax
  void *v153; // rax
  __int64 v154; // rax
  __int64 v155; // rax
  __int64 v156; // rax
  __int64 v157; // rax
  __int64 *v158; // rbx
  __int64 v159; // rdi
  __int64 v160; // rax
  char v161; // bl
  __int64 v162; // rax
  __int64 v163; // rax
  __int64 v164; // rax
  bool v165; // bl
  __int64 v166; // rax
  __int64 v167; // rbx
  __int64 v168; // rax
  void *v169; // rbx
  __int64 v170; // rax
  __int64 v171; // rax
  __int64 v172; // rax
  __int64 v173; // rbx
  void *v174; // rax
  __int64 v175; // rax
  __int64 v176; // rax
  unsigned int v177; // eax
  __int64 v178; // rax
  __int64 v179; // rcx
  __int64 v180; // rcx
  _QWORD *v181; // rbx
  __int64 v182; // rax
  __int64 v183; // rax
  char v184; // bl
  __int64 v185; // rax
  __int64 v186; // rax
  __int64 v187; // rcx
  __int64 v188; // rax
  int v189; // eax
  unsigned int v190; // eax
  _QWORD *v191; // rbx
  __int64 v192; // rax
  _QWORD *v193; // rbx
  __int64 v194; // rax
  _QWORD *v195; // rbx
  __int64 v196; // rax
  __int64 v197; // rbx
  __int64 v198; // rax
  _QWORD *v199; // rbx
  __int64 v200; // rax
  _QWORD *v201; // rbx
  __int64 v202; // rax
  unsigned int v203; // eax
  void *v204; // rax
  void *v205; // rax
  void *v206; // rax
  void *v207; // rax
  void *v208; // rax
  __int64 v209; // r14
  __int64 v210; // rax
  _WORD *v211; // rdi
  __int64 v212; // r14
  unsigned __int64 v213; // rbx
  unsigned int v214; // ebx
  int v215; // eax
  __int64 v216; // rax
  char v217; // r12
  __int64 v218; // rax
  __int64 v219; // rax
  char v220; // al
  char v221; // bl
  __int64 v222; // r8
  unsigned int v223; // ebx
  __int64 v224; // rdx
  __int64 v225; // rax
  __int64 v226; // rax
  void *v227; // rax
  __int64 v228; // rbx
  void *v229; // rax
  char v230; // r14
  __int64 v231; // rax
  __int64 v232; // rbx
  __int64 v233; // rax
  _QWORD *v234; // rax
  __int64 v235; // rax
  __int64 v236; // rax
  __int64 v237; // r13
  __int64 v238; // r8
  __m128d v239; // xmm2
  int v240; // edx
  __int128 v241; // xmm0
  unsigned __int64 v242; // rax
  __int64 v243; // rbx
  __int64 v244; // r14
  __int64 v245; // r12
  _WORD *v246; // rax
  int v247; // ecx
  int v248; // edx
  _WORD *v249; // rdi
  unsigned __int64 v250; // rbx
  int v251; // ebx
  __int64 v252; // rax
  int v253; // eax
  unsigned int v254; // ebx
  int v255; // eax
  __int64 v256; // rax
  __int64 v257; // rax
  __int64 v258; // rbx
  void *v259; // r8
  bool v260; // zf
  __int64 v261; // rbx
  __int64 v262; // rax
  __int64 v263; // rbx
  __int64 v264; // r9
  void *v265; // rdx
  __int64 v266; // rax
  char v267; // al
  __int64 v268; // rdi
  __int64 v269; // rbx
  int v270; // eax
  int v271; // r9d
  int v272; // edx
  __int64 v273; // r15
  __int64 v274; // rdi
  __int64 v275; // rax
  __int64 v276; // rdx
  __int64 v277; // rax
  __int64 v278; // rax
  __int64 v279; // rax
  __int64 v280; // rax
  __int64 v281; // rax
  __int64 v282; // rax
  char v283; // bl
  __int64 v284; // rcx
  __int64 v285; // rax
  __int64 v286; // rdx
  int v287; // r14d
  int *v288; // rdi
  __int64 v289; // rbx
  __int64 v290; // rax
  int v291; // ecx
  __int64 v292; // rax
  char v293; // r12
  __int64 v294; // rax
  __int64 v295; // rax
  __int64 v296; // rax
  __int64 v297; // rbx
  __int64 v298; // rax
  __int64 v299; // rax
  char v300; // r13
  __int128 *v301; // rdx
  __int64 v302; // rbx
  __int64 v303; // rax
  __int64 *v304; // rax
  _DWORD *v305; // rbx
  __int64 v306; // r14
  _DWORD *v307; // rdi
  _DWORD *v308; // rdi
  __m128i v309; // xmm6
  __int64 v310; // rcx
  __int64 v311; // rax
  __int64 v312; // rax
  __int64 v313; // rax
  __int64 v314; // rax
  int v315; // eax
  int v316; // r8d
  int v317; // r9d
  char *v318; // rax
  __m128i v319; // xmm0
  _DWORD *v320; // xmm6_8
  int v321; // eax
  __int64 v322; // rax
  __int64 v323; // rax
  __int64 v324; // rax
  int v325; // edi
  __int64 i; // rbx
  __int64 v327; // rax
  __int64 v328; // rax
  __int64 v329; // rax
  int v330; // edi
  __int64 j; // rbx
  __int64 v332; // rax
  __int64 v333; // rax
  __int64 v334; // rax
  __int64 v335; // rax
  __int64 v337; // r8
  __int64 v338; // rcx
  unsigned __int64 *v339; // [rsp+20h] [rbp-E0h]
  const wchar_t *v340; // [rsp+20h] [rbp-E0h]
  int v341; // [rsp+28h] [rbp-D8h]
  int *v342; // [rsp+28h] [rbp-D8h]
  int v343; // [rsp+30h] [rbp-D0h] BYREF
  int v344; // [rsp+38h] [rbp-C8h]
  __m128i v345; // [rsp+40h] [rbp-C0h] BYREF
  __int64 v346; // [rsp+50h] [rbp-B0h]
  __int64 v347; // [rsp+58h] [rbp-A8h] BYREF
  __int64 v348; // [rsp+60h] [rbp-A0h]
  __int64 v349; // [rsp+68h] [rbp-98h] BYREF
  __int64 v350; // [rsp+70h] [rbp-90h]
  _DWORD v351[6]; // [rsp+78h] [rbp-88h] BYREF
  __int64 v352; // [rsp+90h] [rbp-70h] BYREF
  __int64 v353; // [rsp+98h] [rbp-68h]
  int v354; // [rsp+A0h] [rbp-60h]
  char v355; // [rsp+A4h] [rbp-5Ch]
  char v356; // [rsp+A5h] [rbp-5Bh]
  unsigned __int64 v357; // [rsp+A8h] [rbp-58h] BYREF
  unsigned int v358; // [rsp+B0h] [rbp-50h] BYREF
  _WORD *v359; // [rsp+B8h] [rbp-48h] BYREF
  __int64 v360; // [rsp+C0h] [rbp-40h] BYREF
  int v361; // [rsp+C8h] [rbp-38h]
  int v362; // [rsp+CCh] [rbp-34h]
  __int64 v363; // [rsp+D0h] [rbp-30h] BYREF
  __int64 v364; // [rsp+D8h] [rbp-28h]
  __int64 v365; // [rsp+E0h] [rbp-20h] BYREF
  __int64 v366; // [rsp+E8h] [rbp-18h]
  int v367; // [rsp+F0h] [rbp-10h] BYREF
  __int64 v368; // [rsp+F8h] [rbp-8h]
  __int64 v369; // [rsp+108h] [rbp+8h] BYREF
  int v370; // [rsp+110h] [rbp+10h]
  int v371; // [rsp+114h] [rbp+14h]
  _WORD *v372; // [rsp+118h] [rbp+18h] BYREF
  int v373; // [rsp+120h] [rbp+20h] BYREF
  __int128 v374; // [rsp+128h] [rbp+28h] BYREF
  __m128d v375; // [rsp+138h] [rbp+38h]
  __int64 v376; // [rsp+148h] [rbp+48h] BYREF
  __int64 v377; // [rsp+150h] [rbp+50h]
  _WORD *v378; // [rsp+158h] [rbp+58h] BYREF
  __int128 v379; // [rsp+160h] [rbp+60h] BYREF
  __int128 v380; // [rsp+170h] [rbp+70h]
  void **v381; // [rsp+180h] [rbp+80h]
  __int64 v382[2]; // [rsp+1A0h] [rbp+A0h] BYREF
  int v383; // [rsp+1B0h] [rbp+B0h]
  __int64 v384; // [rsp+1B8h] [rbp+B8h] BYREF
  __int64 v385; // [rsp+1C0h] [rbp+C0h]
  int v386; // [rsp+1C8h] [rbp+C8h]
  __int64 v387[2]; // [rsp+1D0h] [rbp+D0h] BYREF
  __int64 v388[2]; // [rsp+1E0h] [rbp+E0h] BYREF
  _BYTE v389[40]; // [rsp+1F0h] [rbp+F0h] BYREF
  __int64 v390[2]; // [rsp+218h] [rbp+118h] BYREF
  int v391; // [rsp+228h] [rbp+128h]
  char v392[16]; // [rsp+230h] [rbp+130h] BYREF
  char v393[24]; // [rsp+240h] [rbp+140h] BYREF
  char v394[24]; // [rsp+258h] [rbp+158h] BYREF
  char v395[24]; // [rsp+270h] [rbp+170h] BYREF
  char v396[24]; // [rsp+288h] [rbp+188h] BYREF
  char v397[24]; // [rsp+2A0h] [rbp+1A0h] BYREF
  char v398[24]; // [rsp+2B8h] [rbp+1B8h] BYREF
  char v399[24]; // [rsp+2D0h] [rbp+1D0h] BYREF
  char v400[24]; // [rsp+2E8h] [rbp+1E8h] BYREF
  char v401[16]; // [rsp+300h] [rbp+200h] BYREF
  char v402; // [rsp+310h] [rbp+210h] BYREF
  int v403; // [rsp+320h] [rbp+220h] BYREF
  __int64 v404[2]; // [rsp+330h] [rbp+230h] BYREF
  int v405; // [rsp+340h] [rbp+240h]
  char v406; // [rsp+348h] [rbp+248h]
  __m128i v407; // [rsp+350h] [rbp+250h] BYREF
  __int64 v408; // [rsp+360h] [rbp+260h]
  __m128i v409; // [rsp+370h] [rbp+270h] BYREF
  __int64 v410; // [rsp+380h] [rbp+280h]

  v360 = a1;
  v2 = 0;
  v354 = 0;
  v3 = 0;
  sub_1442881A0(0LL);
  sub_140E52110(v400, "FEngineLoop::PreInitPreStartupScreen");
  if ( sub_1442AA870() )
  {
    v5 = sub_1442AA870();
    (*(void (__fastcall **)(__int64))(*(_QWORD *)v5 + 0x68LL))(v5);
  }
  sub_1443011C0();
  sub_1441C6790();
  if ( (unsigned __int8)sub_1442B4720(a2, L"UTF8Output") )
    sub_144301A70();
  sub_14430E120();
  if ( !(unsigned __int8)sub_144256FF0(a2) )
  {
    v6 = 0xFFFFFFFF;
    goto LABEL_538;
  }
  nullsub_1(a2);
  sub_140E52110(v396, "LLM Init");
  sub_140B5A220((__int64)v396);
  sub_140E52110(v397, "InitTaggedStorage");
  sub_140B5A220((__int64)v397);
  sub_140B631C0(off_14A436F10, &v358, &loc_1465DA450);
  sub_140B631C0(&unk_14A42AAF0, &v347, sub_140B79900);
  v376 = 0LL;
  v377 = 0LL;
  sub_140E52110(v393, "LaunchSetGameName");
  if ( !(unsigned __int8)sub_140B6D3E0(a2, &v376) )
  {
    sub_140B5A220((__int64)v393);
    v6 = 1;
    goto LABEL_536;
  }
  sub_140B5A220((__int64)v393);
  sub_140E52110(v399, "CreateConsoleOutputDevice");
  sub_140B5A220((__int64)v399);
  v7 = sub_1442AA870();
  LOBYTE(v8) = 1;
  (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)v7 + 0x60LL))(v7, v8);
  v9 = sub_1442516E0();
  sub_1442B4720(v9, L"stdout");
  sub_14430E120();
  sub_140E52110(v394, "Fix up the relative project path");
  v10 = 0xFFFFFFFFFFFFFFFFuLL;
  if ( (unsigned __int8)sub_1442AEE00() )
  {
    sub_1442AB330(&v407);
    if ( !(unsigned __int8)sub_1418DA9D0(&v407) )
    {
      if ( v407.m128i_i32[2] )
        v11 = (void *)qword_14A426370(&v407);
      else
        v11 = &unk_1479D0BD8;
      sub_14416F7B0(L"Project file not found: %s\n", v11);
      v12 = byte_14A425310;
      if ( (unsigned __int8)byte_14A425310 >= 4u )
      {
        v13 = byte_14A8DE2F8;
        if ( (unsigned __int8)byte_14A8DE2F8 >= 4u )
        {
          if ( v407.m128i_i32[2] )
            v14 = (_WORD *)qword_14A426370(&v407);
          else
            v14 = &unk_1479D0BD8;
          v378 = v14;
          ((void (*)(char *, char *, const wchar_t *, ...))loc_140B5F1A0)(
            (char *)&v343 + 1,
            &byte_14A8DE2F8,
            L"Project file not found: %s",
            "FEngineLoop::PreInitPreStartupScreen",
            &v378);
          v12 = byte_14A425310;
          v13 = byte_14A8DE2F8;
        }
        if ( v12 >= 4u )
        {
          if ( (unsigned __int8)v13 >= 4u )
          {
            sub_140B61230(
              (char *)&v343 + 1,
              &byte_14A8DE2F8,
              L"\tAttempting to find via project info helper.",
              "FEngineLoop::PreInitPreStartupScreen");
            v12 = byte_14A425310;
            v13 = byte_14A8DE2F8;
          }
          if ( v12 >= 4u && (unsigned __int8)v13 >= 4u )
          {
            LODWORD(v357) = v12;
            v339 = &v357;
            sub_140B5FD30(
              (char *)&v343 + 1,
              &byte_14A8DE2F8,
              L"\tCurrent Output Log Level is %d",
              "FEngineLoop::PreInitPreStartupScreen");
          }
        }
      }
      v15 = (_WORD *)sub_144306440();
      v345 = 0uLL;
      v16 = 0;
      v17 = 0;
      v18 = v15;
      if ( v15 && *v15 )
      {
        v19 = 0xFFFFFFFFFFFFFFFFuLL;
        do
          ++v19;
        while ( v15[v19] );
        v20 = v19 + 1;
        if ( v20 > 0 )
        {
          sub_140B5A480(&v345, (unsigned int)v20);
          v17 = v345.m128i_i32[3];
          v16 = v345.m128i_i32[2];
        }
        v345.m128i_i32[2] = v16 + v20;
        if ( v16 + v20 > v17 )
          sub_140B5A3A0(&v345);
        v21 = qword_14A426370(&v345);
        sub_14416FA80(v21, v18, 2LL * v20);
      }
      v22 = sub_1442CB9E0();
      sub_1442CC080(v22, &v409, off_14A3DDDE0, &v345);
      sub_140B5A220((__int64)&v345);
      if ( v409.m128i_i32[2] > 1 )
      {
        if ( (unsigned __int8)byte_14A425310 >= 4u && (unsigned __int8)byte_14A8DE2F8 >= 4u )
        {
          v372 = (_WORD *)qword_14A426370(&v409);
          ((void (*)(char *, char *, const wchar_t *, ...))loc_140B5EF50)(
            (char *)&v343 + 1,
            &byte_14A8DE2F8,
            L"\tFound project file %s.",
            "FEngineLoop::PreInitPreStartupScreen",
            &v372);
        }
        sub_1442BEBD0(&v409);
        v23 = (_WORD *)sub_1442516E0();
        v352 = 0LL;
        v24 = 0;
        v353 = 0LL;
        v25 = 0;
        v26 = v23;
        if ( v23 && *v23 )
        {
          v27 = 0xFFFFFFFFFFFFFFFFuLL;
          do
            ++v27;
          while ( v23[v27] );
          v28 = v27 + 1;
          if ( v28 > 0 )
          {
            sub_140B5A480(&v352, (unsigned int)v28);
            v25 = HIDWORD(v353);
            v24 = v353;
          }
          LODWORD(v353) = v24 + v28;
          if ( v24 + v28 > v25 )
            sub_140B5A3A0(&v352);
          v29 = qword_14A426370(&v352);
          sub_14416FA80(v29, v26, 2LL * v28);
        }
        if ( (_DWORD)v377 )
          v30 = (void *)qword_14A426370(&v376);
        else
          v30 = &unk_1479D0BD8;
        if ( v409.m128i_i32[2] )
          v31 = (void *)qword_14A426370(&v409);
        else
          v31 = &unk_1479D0BD8;
        sub_144154D50(&v352, v30, v31, 0LL);
        if ( (_DWORD)v353 )
          v32 = (void *)qword_14A426370(&v352);
        else
          v32 = &unk_1479D0BD8;
        sub_144256FF0(v32);
        a2 = sub_1442516E0();
        sub_140B5A220((__int64)&v352);
      }
      sub_140B5A220((__int64)&v409);
    }
    sub_140B5A220((__int64)&v407);
  }
  sub_140B5A220((__int64)v394);
  sub_140E52110(v395, "Init Output Devices");
  qword_14A8DEA50 = sub_1445D7AD0();
  qword_14A8D3C90 = sub_1445D7B60();
  sub_140B5A220((__int64)v395);
  sub_140E52110(&v379, "BeginPreInitTextLocalization");
  sub_1441E6870();
  sub_140B5A220((__int64)&v379);
  sub_140E52110(&v367, "LaunchCheckForFileOverride");
  if ( (unsigned __int8)sub_140B6C420(a2, (char *)&v343 + 1) )
  {
    sub_140B5A220((__int64)&v367);
    sub_1442F3420(
      v34,
      v33,
      v35,
      v36,
      (int)v339,
      v341,
      v343,
      v344,
      v345.m128i_i32[0],
      v345.m128i_i32[2],
      v346,
      v347,
      v348,
      v349,
      v350);
    v37 = sub_1442FCA20();
    v373 = 0;
    v38 = v37 <= 4;
    v39 = sub_1442516E0();
    if ( (unsigned __int8)sub_1442C4150(v39, L"LowCPU=", &v373) )
      v38 = v373 != 0;
    if ( dword_14A680B08 > *(_DWORD *)(*((_QWORD *)NtCurrentTeb()->ThreadLocalStoragePointer
                                       + (unsigned int)dword_14AB32F40)
                                     + 0x8DA8LL) )
    {
      sub_147664D94(&dword_14A680B08);
      if ( dword_14A680B08 == 0xFFFFFFFF )
      {
        v338 = qword_14A882320;
        if ( !qword_14A882320 )
        {
          sub_144178740();
          v338 = qword_14A882320;
        }
        LOBYTE(v337) = 1;
        qword_14A680B00 = (*(__int64 (__fastcall **)(__int64, const wchar_t *, __int64))(*(_QWORD *)v338 + 0xB0LL))(
                            v338,
                            L"opt.VeryLowCPU",
                            v337);
        sub_147664D28(&dword_14A680B08);
      }
    }
    if ( qword_14A680B00 )
    {
      v40 = &unk_1479D14F4;
      if ( v38 )
        v40 = &unk_1479D14F0;
      (*(void (__fastcall **)(__int64, void *, __int64))(*(_QWORD *)qword_14A680B00 + 0x80LL))(
        qword_14A680B00,
        v40,
        0x8000000LL);
    }
    else
    {
      byte_14A8DDD21 = v38;
    }
    sub_140E52110(&v374, "IFileManager::Get().ProcessCommandLineOptions");
    v41 = (void (__fastcall ***)(_QWORD))sub_14418C650();
    (**v41)(v41);
    sub_140B5A220((__int64)&v374);
    sub_140E52110(v351, "InitializeNewAsyncIO");
    v42 = sub_1441B0210();
    sub_1441BB600(v42);
    sub_140B5A220((__int64)v351);
    LOBYTE(v43) = 1;
    sub_1442881A0(v43);
    if ( byte_14A821AF4 && (unsigned __int8)sub_140B6D0A0() )
    {
      v44 = 0;
      v362 = 0;
      v360 = 0LL;
      v361 = 0;
      if ( LOWORD(off_14A3DDDE0[0]) )
      {
        do
          ++v10;
        while ( *((_WORD *)off_14A3DDDE0 + v10) );
        v45 = v10 + 1;
        if ( v45 > 0 )
        {
          sub_140B5A480(&v360, (unsigned int)v45);
          v44 = v362;
          v2 = v361;
        }
        v361 = v45 + v2;
        if ( v45 + v2 > v44 )
          sub_140B5A3A0(&v360);
        v46 = qword_14A426370(&v360);
        sub_14416FA80(v46, off_14A3DDDE0, 2LL * v45);
      }
      sub_1441F44A0(&v384, &v360);
      sub_140B5A220((__int64)&v360);
      v47 = v385;
      v48 = v384;
      v390[0] = v384;
      v390[1] = v385;
      if ( v385 )
        _InterlockedIncrement((volatile signed __int32 *)(v385 + 8));
      v49 = v386;
      v391 = v386;
      v50 = sub_1441AF230(
              v389,
              L"Error: UE4Editor does not append 'Game' to the passed in game name.\n"
               "You must use the full name.\n"
               "You specified '{0}', use '{0}Game'.",
              L"LaunchEngineLoop",
              L"RequiresGamePrefix");
      v51 = (__int64 *)sub_1441D68E0(v401, v50);
      v403 = 4;
      v52 = v51;
      v404[0] = v48;
      v404[1] = v47;
      if ( v47 )
        _InterlockedIncrement((volatile signed __int32 *)(v47 + 8));
      v405 = v49;
      v387[0] = (__int64)&v403;
      v387[1] = (__int64)&v407;
      v388[0] = *v51;
      v53 = v51[1];
      v406 = 1;
      v388[1] = v53;
      if ( v53 )
        _InterlockedIncrement((volatile signed __int32 *)(v53 + 8));
      v54 = sub_140B643A0(v392, v387);
      v55 = sub_1441F2970(v398, v388, v54);
      v382[0] = *(_QWORD *)v55;
      v56 = *(_QWORD *)(v55 + 8);
      v382[1] = v56;
      if ( v56 )
        _InterlockedIncrement((volatile signed __int32 *)(v56 + 8));
      v383 = *(_DWORD *)(v55 + 0x10);
      sub_140B65240(v398);
      sub_140B65010(v392);
      sub_140B65180(v404);
      sub_140B65240(v52);
      sub_140B65240(v390);
      sub_1442B4110(0LL, v382, 0LL);
      sub_140B65240(v382);
      sub_140B65240(v389);
      sub_140B65240(&v384);
      v6 = 1;
      goto LABEL_536;
    }
    dword_14A8DDCFC = MEMORY[0x31FFDF57]();
    byte_14A8DDD10 = 1;
    sub_14430CD40();
    sub_14430E140(qword_14A434540);
    sub_14430CD40();
    sub_14430E170(qword_14A434540);
    v57 = 0xFFFFFFFFFFFFFFFFuLL;
    do
      ++v57;
    while ( *(_WORD *)(a2 + 2 * v57) );
    v58 = (_WORD *)sub_144020460(saturated_mul((int)v57 + 1, 2uLL));
    v59 = (__int64)v58;
    v372 = v58;
    v60 = v58;
    v61 = a2 - (_QWORD)v58;
    do
    {
      v62 = *(_WORD *)((char *)v60 + v61);
      *v60++ = v62;
    }
    while ( v62 );
    v359 = v58;
    sub_1442C09E0(&v349, &v359, 0LL);
    v63 = (_WORD *)sub_144178850();
    v352 = 0LL;
    v64 = 0;
    v353 = 0LL;
    v65 = 0;
    v66 = v63;
    if ( v63 && *v63 )
    {
      v67 = 0xFFFFFFFFFFFFFFFFuLL;
      do
        ++v67;
      while ( v63[v67] );
      v68 = v67 + 1;
      if ( v68 > 0 )
      {
        sub_140B5A480(&v352, (unsigned int)v68);
        v65 = HIDWORD(v353);
        v64 = v353;
      }
      LODWORD(v353) = v68 + v64;
      if ( v68 + v64 > v65 )
        sub_140B5A3A0(&v352);
      v69 = qword_14A426370(&v352);
      sub_14416FA80(v69, v66, 2LL * v68);
      v59 = (__int64)v372;
    }
    v345 = 0uLL;
    sub_140B5A480(&v345, 8LL);
    v345.m128i_i32[2] += 8;
    if ( v345.m128i_i32[2] > v345.m128i_i32[3] )
      sub_140B5A3A0(&v345);
    v70 = qword_14A426370(&v345);
    sub_14416FA80(v70, L"/Engine", 0x10LL);
    sub_14586DD90(&v345, &v352);
    sub_140B5A220((__int64)&v345);
    sub_140B5A220((__int64)&v352);
    v365 = 0LL;
    v366 = 0LL;
    v363 = 0LL;
    v364 = 0LL;
    sub_140B6E6C0(v59, &v365, &v363);
    v71 = v366;
    v72 = 0;
    LOBYTE(v343) = 0;
    v73 = 0;
    if ( (int)v366 > 0 )
    {
      while ( 1 )
      {
        if ( v73 < 0 || v73 >= v71 )
          MEMORY[0x10] = 0x5474736172434155LL;
        v74 = qword_14A426370(&v365);
        if ( (unsigned __int8)sub_144147510(v74 + 0x10LL * v73, L"Commandlet", 1LL) )
          break;
        v71 = v366;
        if ( ++v73 >= (int)v366 )
          goto LABEL_114;
      }
      v72 = 1;
      LOBYTE(v343) = 1;
      if ( v73 < 0 || v73 >= (int)v366 )
        MEMORY[0x10] = 0x5474736172434155LL;
      v75 = qword_14A426370(&v365);
      sub_140B5CE20(&v349, v75 + 0x10LL * v73);
    }
LABEL_114:
    v76 = v364;
    if ( (int)v364 <= 0 )
    {
      if ( !v72 )
        goto LABEL_128;
    }
    else
    {
      v77 = 0;
      if ( !v72 )
      {
        while ( 1 )
        {
          if ( v77 < 0 || v77 >= v76 )
            MEMORY[0x10] = 0x5474736172434155LL;
          v78 = qword_14A426370(&v363);
          if ( (unsigned __int8)sub_144156A70(v78 + 0x10LL * v77, L"RUN=", 1LL) )
            break;
          v76 = v364;
          if ( ++v77 >= (int)v364 )
            goto LABEL_128;
        }
        LOBYTE(v343) = 1;
        if ( v77 < 0 || v77 >= (int)v364 )
          MEMORY[0x10] = 0x5474736172434155LL;
        v79 = qword_14A426370(&v363);
        sub_140B5CE20(&v349, v79 + 0x10LL * v77);
      }
    }
    byte_14A8DDCA6 = 1;
LABEL_128:
    sub_144157C60(&v349);
    v369 = 0LL;
    v80 = qword_14A426370(&v349);
    v81 = (int)v350;
    v82 = v80;
    v370 = v350;
    if ( (_DWORD)v350 )
    {
      sub_140B77D60(&v369, (unsigned int)v350, 0LL);
      v83 = qword_14A426370(&v369);
      sub_14766AF37(v83, v82, 2 * v81);
    }
    else
    {
      v371 = 0;
    }
    sub_1442B40F0(&v369);
    v85 = 0;
    if ( LOWORD(off_14A3DDDE0[0]) && (unsigned int)sub_144179300(off_14A3DDDE0, L"None") )
    {
      v84 = (_DWORD)v350 ? (void *)qword_14A426370(&v349) : &unk_1479D0BD8;
      if ( !(unsigned int)sub_144179300(v84, off_14A3DDDE0) )
        v85 = 1;
    }
    if ( (unsigned __int8)sub_1442AEE00() )
    {
      v86 = sub_1442AB330(&v407);
      v87 = *(_DWORD *)(v86 + 8);
      if ( v370 == v87 )
      {
        if ( v370 <= 1 )
        {
LABEL_145:
          v3 = 1;
          v91 = 1;
          v354 = 1;
LABEL_147:
          if ( (v3 & 1) != 0 )
          {
            v3 &= ~1u;
            v354 = v3;
            qword_14A426370(&v407);
            sub_140B5A380(&v407, 0LL);
            if ( qword_14A426370(&v407) )
            {
              v92 = qword_14A426370(&v407);
              sub_1441AFDF0(v92);
            }
          }
          if ( (unsigned __int8)sub_1442AEE00() )
          {
            v3 |= 6u;
            v354 = v3;
            v93 = sub_1442AB330(&v409);
            v94 = sub_1442A91D0(&v407, v93);
            v95 = *(_DWORD *)(v94 + 8);
            if ( (_DWORD)v350 == v95 )
            {
              if ( (int)v350 <= 1 )
              {
LABEL_156:
                v99 = 1;
LABEL_158:
                if ( (v3 & 4) != 0 )
                {
                  v3 &= ~4u;
                  v354 = v3;
                  qword_14A426370(&v407);
                  sub_140B5A380(&v407, 0LL);
                  if ( qword_14A426370(&v407) )
                  {
                    v100 = qword_14A426370(&v407);
                    sub_1441AFDF0(v100);
                  }
                }
                if ( (v3 & 2) != 0 )
                {
                  v3 &= ~2u;
                  v354 = v3;
                  qword_14A426370(&v409);
                  sub_140B5A380(&v409, 0LL);
                  if ( qword_14A426370(&v409) )
                  {
                    v101 = qword_14A426370(&v409);
                    sub_1441AFDF0(v101);
                  }
                }
                if ( v85 || v91 || v99 )
                {
                  v102 = v359;
                  v103 = 0;
                  v347 = 0LL;
                  v104 = 0;
                  v348 = 0LL;
                  if ( !v359 || !*v359 )
                    goto LABEL_177;
                  v105 = 0xFFFFFFFFFFFFFFFFuLL;
                  do
                    ++v105;
                  while ( v359[v105] );
                  v106 = v105 + 1;
                  if ( v106 > 0 )
                  {
                    sub_140B5A480(&v347, (unsigned int)v106);
                    v104 = HIDWORD(v348);
                    v103 = v348;
                  }
                  LODWORD(v348) = v106 + v103;
                  if ( v106 + v103 > v104 )
                    sub_140B5A3A0(&v347);
                  v107 = qword_14A426370(&v347);
                  sub_14416FA80(v107, v102, 2LL * v106);
                  if ( (_DWORD)v348 )
                    v108 = (char *)qword_14A426370(&v347);
                  else
LABEL_177:
                    v108 = (char *)&unk_1479D0BD8;
                  v109 = v372;
                  v110 = (char *)v372 - v108;
                  do
                  {
                    v111 = *(_WORD *)v108;
                    *(_WORD *)&v108[v110] = *(_WORD *)v108;
                    v108 += 2;
                  }
                  while ( v111 );
                  v359 = v109;
                  sub_144256FF0(v109);
                  v112 = (__int64 *)sub_1442C09E0(&v407, &v359, 0LL);
                  if ( &v349 != v112 )
                  {
                    qword_14A426370(&v349);
                    if ( qword_14A426370(&v349) )
                    {
                      v113 = qword_14A426370(&v349);
                      sub_1441AFDF0(v113);
                    }
                    v349 = *v112;
                    qword_14A426370(&v349);
                    *v112 = 0LL;
                    qword_14A426370(v112);
                    v350 = v112[1];
                    v112[1] = 0LL;
                  }
                  qword_14A426370(&v407);
                  sub_140B5A380(&v407, 0LL);
                  if ( qword_14A426370(&v407) )
                  {
                    v114 = (__int128 *)&v407;
LABEL_186:
                    v115 = qword_14A426370(v114);
                    sub_1441AFDF0(v115);
                    goto LABEL_187;
                  }
                  while ( 1 )
                  {
LABEL_187:
                    sub_144157C80(&v349);
                    v116 = sub_14431CD20(&v409);
                    v117 = sub_1442A9D10(&v407, &v349, 0LL);
                    v118 = *(_DWORD *)(v116 + 8);
                    v119 = v117;
                    v120 = *(_DWORD *)(v117 + 8);
                    if ( v120 == v118 )
                    {
                      if ( v120 <= 1 )
                      {
                        v121 = 1;
                      }
                      else
                      {
                        v122 = qword_14A426370(v116);
                        v123 = qword_14A426370(v119);
                        v121 = (unsigned int)sub_144179300(v123, v122) == 0;
                      }
                    }
                    else
                    {
                      v121 = v118 + v120 == 1;
                    }
                    qword_14A426370(&v407);
                    sub_140B5A380(&v407, 0LL);
                    if ( qword_14A426370(&v407) )
                    {
                      v124 = qword_14A426370(&v407);
                      sub_1441AFDF0(v124);
                    }
                    qword_14A426370(&v409);
                    sub_140B5A380(&v409, 0LL);
                    if ( qword_14A426370(&v409) )
                    {
                      v125 = qword_14A426370(&v409);
                      sub_1441AFDF0(v125);
                    }
                    if ( !v121 )
                      break;
                    v126 = (__int64 *)sub_1442C09E0(&v345, &v359, 0LL);
                    if ( &v349 != v126 )
                    {
                      qword_14A426370(&v349);
                      if ( qword_14A426370(&v349) )
                      {
                        v127 = qword_14A426370(&v349);
                        sub_1441AFDF0(v127);
                      }
                      v349 = *v126;
                      qword_14A426370(&v349);
                      *v126 = 0LL;
                      qword_14A426370(v126);
                      v350 = v126[1];
                      v126[1] = 0LL;
                    }
                    qword_14A426370(&v345);
                    sub_140B5A380(&v345, 0LL);
                    if ( qword_14A426370(&v345) )
                    {
                      v114 = (__int128 *)&v345;
                      goto LABEL_186;
                    }
                  }
                  if ( v91 || v99 )
                  {
                    v128 = sub_1442AB330(&v407);
                    if ( *(_DWORD *)(v128 + 8) )
                      v129 = (void *)qword_14A426370(v128);
                    else
                      v129 = &unk_1479D0BD8;
                    sub_1441874A0(&v345, v129);
                    qword_14A426370(&v407);
                    sub_140B5A380(&v407, 0LL);
                    if ( qword_14A426370(&v407) )
                    {
                      v130 = qword_14A426370(&v407);
                      sub_1441AFDF0(v130);
                    }
                    v131 = sub_1442AB330(&v407);
                    v132 = *(_DWORD *)(v131 + 8);
                    if ( v345.m128i_i32[2] == v132 )
                    {
                      if ( v345.m128i_i32[2] <= 1 )
                      {
                        v133 = 0;
                      }
                      else
                      {
                        v134 = qword_14A426370(v131);
                        v135 = qword_14A426370(&v345);
                        v133 = (unsigned int)sub_144179300(v135, v134) != 0;
                      }
                    }
                    else
                    {
                      v133 = v132 + v345.m128i_i32[2] != 1;
                    }
                    qword_14A426370(&v407);
                    sub_140B5A380(&v407, 0LL);
                    if ( qword_14A426370(&v407) )
                    {
                      v136 = qword_14A426370(&v407);
                      sub_1441AFDF0(v136);
                    }
                    if ( v133 )
                      sub_1442BEBD0(&v345);
                    qword_14A426370(&v345);
                    sub_140B5A380(&v345, 0LL);
                    if ( qword_14A426370(&v345) )
                    {
                      v137 = qword_14A426370(&v345);
                      sub_1441AFDF0(v137);
                    }
                  }
                  qword_14A426370(&v347);
                  sub_140B5A380(&v347, 0LL);
                  if ( qword_14A426370(&v347) )
                  {
                    v138 = qword_14A426370(&v347);
                    sub_1441AFDF0(v138);
                  }
                }
                v378 = 0LL;
                if ( (_BYTE)v343 )
                {
                  if ( (unsigned __int8)sub_144156A70(&v349, L"run=", 1LL) )
                  {
                    if ( (_DWORD)v350 )
                    {
                      v139 = v350 - 1;
                      if ( (int)v350 - 1 > 4 )
                        v139 = 4;
                      if ( v139 )
                      {
                        qword_14A426370(&v349);
                        v140 = v350;
                        v141 = v350 - v139;
                        if ( (_DWORD)v350 != v139 )
                        {
                          v142 = qword_14A426370(&v349);
                          v143 = qword_14A426370(&v349);
                          sub_14766AF3D(v143, v142 + 2LL * v139, 2LL * v141);
                          v140 = v350;
                        }
                        LODWORD(v350) = v140 - v139;
                      }
                    }
                    if ( !(unsigned __int8)sub_144147510(&v349, L"Commandlet", 1LL) )
                      sub_144144330(&v349, L"Commandlet", 0xALL);
                  }
                  v378 = v359;
                }
                byte_14A8941A1 = 0;
                v144 = sub_1442516E0();
                v145 = sub_1442B4720(v144, L"Deterministic");
                if ( v145 || (v146 = sub_1442516E0(), (unsigned __int8)sub_1442B4720(v146, L"UseFixedTimeStep")) )
                {
                  byte_14A8941A2 = 1;
                  if ( v145 )
                    goto LABEL_242;
                }
                else
                {
                  byte_14A8941A2 = 0;
                }
                if ( !byte_14A8941A1 )
                {
                  v147 = sub_1442516E0();
                  if ( !(unsigned __int8)sub_1442B4720(v147, L"FixedSeed") )
                  {
                    byte_14A8941A0 = 0;
                    MEMORY[0x31FFDDE7](&v357);
                    v148 = v357;
                    MEMORY[0x31FFDDE7](&v358);
                    v149 = v358;
                    goto LABEL_243;
                  }
                }
LABEL_242:
                v148 = 0;
                byte_14A8941A0 = 1;
                LODWORD(v357) = 0;
                v149 = 0;
                v358 = 0;
LABEL_243:
                MEMORY[0x7FF9466E6120](v148);
                sub_144174DB0(v149);
                if ( (unsigned __int8)byte_14A425310 >= 6u && (unsigned __int8)byte_14A8DE2F8 >= 6u )
                {
                  v342 = (int *)&v358;
                  v340 = (const wchar_t *)&v357;
                  ((void (__fastcall *)(char *, char *, const wchar_t *, const char *))loc_140B5F630)(
                    (char *)&v343 + 1,
                    &byte_14A8DE2F8,
                    L"RandInit(%d) SRandInit(%d).",
                    "FEngineLoop::PreInitPreStartupScreen");
                }
                if ( !byte_14A821AF4
                  && LOWORD(off_14A3DDDE0[0])
                  && (unsigned int)sub_144179300(off_14A3DDDE0, L"None")
                  && !(unsigned __int8)sub_1442AEE00() )
                {
                  v150 = sub_14431CD20(&v352);
                  if ( *(_DWORD *)(v150 + 8) )
                    qword_14A426370(v150);
                  sub_144152E40(&v407, L"%s.%s", off_14A3DDDE0);
                  if ( v407.m128i_i32[2] )
                    v151 = (void *)qword_14A426370(&v407);
                  else
                    v151 = &unk_1479D0BD8;
                  v152 = sub_1442B8A90(&v409);
                  if ( *(_DWORD *)(v152 + 8) )
                    v153 = (void *)qword_14A426370(v152);
                  else
                    v153 = &unk_1479D0BD8;
                  v347 = (__int64)v153;
                  v348 = (__int64)v151;
                  v345 = 0uLL;
                  sub_14429AE40(&v345, &v347, 2LL);
                  qword_14A426370(&v409);
                  sub_140B5A380(&v409, 0LL);
                  if ( qword_14A426370(&v409) )
                  {
                    v154 = qword_14A426370(&v409);
                    sub_1441AFDF0(v154);
                  }
                  qword_14A426370(&v407);
                  sub_140B5A380(&v407, 0LL);
                  if ( qword_14A426370(&v407) )
                  {
                    v155 = qword_14A426370(&v407);
                    sub_1441AFDF0(v155);
                  }
                  qword_14A426370(&v352);
                  sub_140B5A380(&v352, 0LL);
                  if ( qword_14A426370(&v352) )
                  {
                    v156 = qword_14A426370(&v352);
                    sub_1441AFDF0(v156);
                  }
                  sub_1442BEBD0(&v345);
                  qword_14A426370(&v345);
                  sub_140B5A380(&v345, 0LL);
                  if ( qword_14A426370(&v345) )
                  {
                    v157 = qword_14A426370(&v345);
                    sub_1441AFDF0(v157);
                  }
                }
                if ( (unsigned __int8)sub_1442AEE00() )
                {
                  sub_140E52110(&v367, "IProjectManager::Get().LoadProjectFile");
                  v158 = (__int64 *)sub_14431B310();
                  v159 = *v158;
                  v160 = sub_1442AB330(&v407);
                  v161 = (*(__int64 (__fastcall **)(__int64 *, __int64))(v159 + 0x10))(v158, v160);
                  qword_14A426370(&v407);
                  sub_140B5A380(&v407, 0LL);
                  if ( qword_14A426370(&v407) )
                  {
                    v162 = qword_14A426370(&v407);
                    sub_1441AFDF0(v162);
                  }
                  if ( !v161 )
                  {
                    if ( (unsigned __int8)byte_14A425310 >= 3u && (unsigned __int8)byte_14A8DE2F8 >= 3u )
                      sub_140B61FF0(
                        (char *)&v343 + 1,
                        &byte_14A8DE2F8,
                        L"Could not find a valid project file, the engine will exit now.",
                        "FEngineLoop::PreInitPreStartupScreen");
LABEL_273:
                    sub_140B5A220((__int64)&v367);
                    v6 = 1;
                    goto LABEL_520;
                  }
                  v163 = sub_14431B310();
                  v165 = 0;
                  if ( (*(unsigned __int8 (__fastcall **)(__int64))(*(_QWORD *)v163 + 0xA8LL))(v163) )
                  {
                    v3 |= 8u;
                    v354 = v3;
                    v164 = sub_1442A30C0(&v407);
                    if ( (unsigned __int8)sub_1418DA980(v164) )
                      v165 = 1;
                  }
                  if ( (v3 & 8) != 0 )
                  {
                    v3 &= ~8u;
                    v354 = v3;
                    qword_14A426370(&v407);
                    sub_140B5A380(&v407, 0LL);
                    if ( qword_14A426370(&v407) )
                    {
                      v166 = qword_14A426370(&v407);
                      sub_1441AFDF0(v166);
                    }
                  }
                  if ( v165 )
                  {
                    v167 = sub_14416D830();
                    v168 = sub_1442A30C0(&v407);
                    if ( *(_DWORD *)(v168 + 8) )
                      v409.m128i_i64[0] = qword_14A426370(v168);
                    else
                      v409.m128i_i64[0] = (__int64)&unk_1479D0BD8;
                    v409.m128i_i64[1] = (__int64)L"Binaries";
                    v410 = v167;
                    v345 = 0uLL;
                    sub_14429AE40(&v345, &v409, 3LL);
                    if ( v345.m128i_i32[2] )
                      v169 = (void *)qword_14A426370(&v345);
                    else
                      v169 = &unk_1479D0BD8;
                    v170 = sub_1442CB980();
                    sub_1442C9910(v170, v169, 0LL);
                    qword_14A426370(&v345);
                    sub_140B5A380(&v345, 0LL);
                    if ( qword_14A426370(&v345) )
                    {
                      v171 = qword_14A426370(&v345);
                      sub_1441AFDF0(v171);
                    }
                    qword_14A426370(&v407);
                    sub_140B5A380(&v407, 0LL);
                    if ( qword_14A426370(&v407) )
                    {
                      v172 = qword_14A426370(&v407);
                      sub_1441AFDF0(v172);
                    }
                  }
                  sub_140B5A220((__int64)&v367);
                }
                sub_140B6CD80();
                if ( LOWORD(off_14A3DDDE0[0]) && (unsigned int)sub_144179300(off_14A3DDDE0, L"None") )
                {
                  v173 = sub_14416D830();
                  v407.m128i_i64[0] = sub_144171550();
                  v407.m128i_i64[1] = (__int64)L"Binaries";
                  v408 = v173;
                  v352 = 0LL;
                  v353 = 0LL;
                  sub_14429AE40(&v352, &v407, 3LL);
                  if ( (_DWORD)v353 )
                    v174 = (void *)qword_14A426370(&v352);
                  else
                    v174 = &unk_1479D0BD8;
                  sub_144305E90(v174);
                  if ( (_DWORD)v353 )
                    qword_14A426370(&v352);
                  v175 = sub_1442CB980();
                  nullsub_1(v175);
                  sub_140B6C600();
                  qword_14A426370(&v352);
                  sub_140B5A380(&v352, 0LL);
                  if ( qword_14A426370(&v352) )
                  {
                    v176 = qword_14A426370(&v352);
                    sub_1441AFDF0(v176);
                  }
                }
                sub_140E52110(v351, "FTaskGraphInterface::Startup");
                v177 = sub_1442FCA20();
                sub_144156B40(v177);
                v178 = sub_1441482A0();
                (*(void (__fastcall **)(__int64, __int64))(*(_QWORD *)v178 + 0x28LL))(v178, 2LL);
                sub_140B5A220((__int64)v351);
                LOBYTE(v179) = 4;
                sub_1442881A0(v179);
                LOBYTE(v180) = 2;
                sub_1442881A0(v180);
                sub_140E52110(&v367, "LoadCoreModules");
                v181 = (_QWORD *)sub_1442E8E80(&v347, L"CoreUObject", 1LL);
                v182 = sub_1442CB980();
                if ( !sub_1442CCDB0(v182, *v181) )
                {
                  if ( (unsigned __int8)byte_14A425310 >= 2u && (unsigned __int8)byte_14A8DE2F8 >= 2u )
                    sub_140B5FAF0(
                      (char *)&v343 + 1,
                      &byte_14A8DE2F8,
                      L"Failed to load Core modules.",
                      "FEngineLoop::PreInitPreStartupScreen");
                  goto LABEL_273;
                }
                sub_140B5A220((__int64)&v367);
                v183 = sub_1442516E0();
                v184 = sub_1442B4720(v183, L"DumpEarlyConfigReads");
                v355 = v184;
                v185 = sub_1442516E0();
                v356 = sub_1442B4720(v185, L"DumpEarlyPakFileReads");
                v186 = sub_1442516E0();
                BYTE1(v343) = sub_1442B4720(v186, L"ForceQuitAfterEarlyReads");
                if ( v184 )
                  nullsub_1(v187);
                sub_146885450();
                if ( (unsigned int)v350 < 2
                  || (v188 = qword_14A426370(&v349), v189 = sub_144179500(v188, "-", 1LL), BYTE1(v346) = 1, !v189) )
                {
                  BYTE1(v346) = 0;
                }
                if ( (unsigned __int8)sub_144179640() )
                {
                  sub_140E52110(v351, "Init FQueuedThreadPool's");
                  qword_14A88BD18 = sub_1441A52A0();
                  v190 = sub_1442FCE20();
                  (**(void (__fastcall ***)(__int64, _QWORD, __int64, __int64, const wchar_t *))qword_14A88BD18)(
                    qword_14A88BD18,
                    v190,
                    0x20000LL,
                    5LL,
                    L"ThreadPool");
                  qword_14A88BD28 = sub_1441A52A0();
                  v340 = L"BackgroundThreadPool";
                  (**(void (__fastcall ***)(__int64, __int64, __int64, __int64))qword_14A88BD28)(
                    qword_14A88BD28,
                    2LL,
                    0x20000LL,
                    4LL);
                  sub_140B5A220((__int64)v351);
                }
                qword_14A8D3C88 = qword_14A6809E8;
                sub_140E52110(v351, "LoadPreInitModules");
                v191 = (_QWORD *)sub_1442E8E80(&v347, L"Engine", 1LL);
                v192 = sub_1442CB980();
                sub_1442CCDB0(v192, *v191);
                v193 = (_QWORD *)sub_1442E8E80(&v357, L"Renderer", 1LL);
                v194 = sub_1442CB980();
                sub_1442CCDB0(v194, *v193);
                v195 = (_QWORD *)sub_1442E8E80(&v358, L"AnimGraphRuntime", 1LL);
                v196 = sub_1442CB980();
                sub_1442CCDB0(v196, *v195);
                sub_1445DCB40();
                if ( !byte_14AA4E2C8 )
                {
                  sub_1442CB980();
                  v197 = *(_QWORD *)sub_1442E8D60(&v347, "SlateRHIRenderer", 1LL);
                  v198 = sub_1442CB980();
                  sub_1442CCF00(v198, v197);
                }
                v199 = (_QWORD *)sub_1442E8E80(&v347, L"Landscape", 1LL);
                v200 = sub_1442CB980();
                sub_1442CCDB0(v200, *v199);
                v201 = (_QWORD *)sub_1442E8E80(&v357, L"RenderCore", 1LL);
                v202 = sub_1442CB980();
                sub_1442CCDB0(v202, *v201);
                sub_140B5A220((__int64)v351);
                sub_140B631C0(&unk_14A42ACD0, &v347, qword_140B67E60);
                sub_140B631C0(&unk_14A42ACE8, &v347, qword_140B67300);
                sub_140B631C0(&unk_14A42AD00, &v347, qword_140B67EA0);
                sub_140B631C0(&unk_14A42AD18, &v347, sub_140B672C0);
                sub_140E52110(&v367, "AppInit");
                if ( !((unsigned __int8 (*)(void))&qword_140B67300[8])() )
                {
LABEL_465:
                  sub_140B5A220((__int64)&v367);
                  v6 = 1;
                  goto LABEL_520;
                }
                sub_140B5A220((__int64)&v367);
                if ( (unsigned __int8)sub_144179640() )
                {
                  sub_140E52110(v351, "GIOThreadPool->Create");
                  qword_14A88BD20 = sub_1441A52A0();
                  v203 = sub_140EB3AE0();
                  v340 = L"IOThreadPool";
                  (**(void (__fastcall ***)(__int64, _QWORD, __int64, __int64))qword_14A88BD20)(
                    qword_14A88BD20,
                    v203,
                    0x18000LL,
                    1LL);
                  sub_140B5A220((__int64)v351);
                }
                nullsub_1(1LL);
                sub_140E52110(v351, "System settings and cvar init");
                sub_14683F100(off_14A48CC10, 0LL);
                if ( dword_14A8DE098 )
                  v204 = (void *)qword_14A426370(&unk_14A8DE090);
                else
                  v204 = &unk_1479D0BD8;
                sub_144268FE0(L"/Script/Engine.RendererSettings", v204, 0x3000000LL, 0LL);
                if ( dword_14A8DE098 )
                  v205 = (void *)qword_14A426370(&unk_14A8DE090);
                else
                  v205 = &unk_1479D0BD8;
                sub_144268FE0(L"/Script/Engine.RendererOverrideSettings", v205, 0x3000000LL, 0LL);
                if ( dword_14A8DE098 )
                  v206 = (void *)qword_14A426370(&unk_14A8DE090);
                else
                  v206 = &unk_1479D0BD8;
                sub_144268FE0(L"/Script/Engine.StreamingSettings", v206, 0x3000000LL, 0LL);
                if ( dword_14A8DE098 )
                  v207 = (void *)qword_14A426370(&unk_14A8DE090);
                else
                  v207 = &unk_1479D0BD8;
                sub_144268FE0(L"/Script/Engine.GarbageCollectionSettings", v207, 0x3000000LL, 0LL);
                if ( dword_14A8DE098 )
                  v208 = (void *)qword_14A426370(&unk_14A8DE090);
                else
                  v208 = &unk_1479D0BD8;
                sub_144268FE0(L"/Script/Engine.NetworkSettings", v208, 0x3000000LL, 0LL);
                if ( !byte_14A8DDCA6 )
                  sub_14625D8E0();
                sub_140B5A220((__int64)v351);
                sub_140E52110(v351, "InitScalabilitySystem");
                sub_1466BF080();
                sub_140B5A220((__int64)v351);
                sub_140E52110(v351, "InitializeCVarsForActiveDeviceProfile");
                sub_1461E3490(0LL, 0LL);
                sub_140B5A220((__int64)v351);
                sub_140E52110(v351, "Scalability::LoadState");
                sub_1466C06C0(&unk_14A8DE150);
                sub_140B5A220((__int64)v351);
                if ( (unsigned __int8)sub_144179DC0() )
                  byte_14AA60849 = 1;
                sub_140E52110(v351, "LoadConsoleVariablesFromINI");
                sub_14427C520();
                sub_140B5A220((__int64)v351);
                sub_140E52110(v351, "Platform Initialization");
                sub_1442FD6D0();
                sub_1445DB3A0();
                sub_1442F92E0();
                sub_140B5A220((__int64)v351);
                if ( (unsigned __int8)sub_14422F7F0() )
                {
                  sub_140E52110(v351, "InitIoDispatcher");
                  sub_14422F6E0();
                  sub_140B5A220((__int64)v351);
                }
                v209 = qword_14A8D3C88;
                if ( qword_14A8D3C88 )
                {
                  if ( dword_14A8DE148 )
                  {
                    v210 = qword_14A426370(&unk_14A8DE140);
                    v209 = qword_14A8D3C88;
                    v211 = (_WORD *)v210;
                  }
                  else
                  {
                    v211 = &unk_1479D0BD8;
                  }
                  v212 = v209 + 0x10;
                  if ( (_WORD *)qword_14A426370(v212) != v211 )
                  {
                    if ( v211 && *v211 )
                    {
                      v213 = 0xFFFFFFFFFFFFFFFFuLL;
                      do
                        ++v213;
                      while ( v211[v213] );
                      v214 = v213 + 1;
                    }
                    else
                    {
                      v214 = 0;
                    }
                    qword_14A426370(v212);
                    *(_DWORD *)(v212 + 8) = 0;
                    if ( *(_DWORD *)(v212 + 0xC) != v214 )
                      sub_140B5A480(v212, v214);
                    v215 = *(_DWORD *)(v212 + 8) + v214;
                    *(_DWORD *)(v212 + 8) = v215;
                    if ( v215 > *(_DWORD *)(v212 + 0xC) )
                      sub_140B5A3A0(v212);
                    if ( v214 )
                    {
                      v216 = qword_14A426370(v212);
                      sub_14766AF37(v216, v211, 2LL * (int)v214);
                    }
                  }
                }
                nullsub_1(2LL);
                if ( !(unsigned __int8)sub_1442EEA90() )
                  sub_1443000F0(0LL);
                v217 = 0;
                byte_14A8DDCA9 = 1;
                LOBYTE(v346) = 0;
                byte_14A8DDCAA = 1;
                byte_14A8DDCA6 = 1;
                v218 = sub_1442516E0();
                byte_14A8DDCA7 = sub_1442B4720(v218, L"AllowCommandletRendering");
                v219 = sub_1442516E0();
                v220 = sub_1442B4720(v219, L"AllowCommandletAudio");
                v221 = BYTE1(v346);
                byte_14A8DDCA8 = v220;
                if ( !BYTE1(v346) )
                  goto LABEL_553;
                if ( (unsigned __int8)sub_144147510(&v349, L"Commandlet", 1LL) )
                  goto LABEL_386;
                if ( !v221 )
                {
LABEL_553:
                  if ( (unsigned __int8)sub_144156A70(&v349, L"run=", 1LL) )
                  {
                    if ( (_DWORD)v350 )
                      v222 = (unsigned int)(v350 - 1);
                    else
                      v222 = 0LL;
                    if ( (int)v222 > 4 )
                      v222 = 4LL;
                    sub_140B770E0(&v349, 0LL, v222, 0LL);
                    if ( (unsigned __int8)sub_144147510(&v349, L"Commandlet", 1LL) )
                      goto LABEL_386;
                    goto LABEL_370;
                  }
                  goto LABEL_385;
                }
                v347 = 0LL;
                v348 = 0LL;
                if ( (_DWORD)v350 )
                {
                  v223 = v350 - 1;
                  if ( (int)v350 - 1 + 0xA <= 0 || (v224 = (unsigned int)(v350 + 0xA), (int)v224 <= 0) )
                  {
LABEL_377:
                    if ( v223 )
                    {
                      v225 = qword_14A426370(&v349);
                      sub_144144330(&v347, v225, v223);
                    }
                    sub_144144330(&v347, L"Commandlet", 0xALL);
                    v345.m128i_i64[0] = 0LL;
                    if ( qword_14A426370(&v345) )
                    {
                      v226 = qword_14A426370(&v345);
                      sub_1441AFDF0(v226);
                    }
                    v345.m128i_i64[0] = v347;
                    qword_14A426370(&v345);
                    v347 = 0LL;
                    qword_14A426370(&v347);
                    v345.m128i_i64[1] = v348;
                    v348 = 0LL;
                    sub_140B5A220((__int64)&v347);
                    if ( v345.m128i_i32[2] )
                      v227 = (void *)qword_14A426370(&v345);
                    else
                      v227 = &unk_1479D0BD8;
                    v228 = sub_140B63F10(0xFFFFFFFFFFFFFFFFuLL, v227, 0LL);
                    sub_140B5A220((__int64)&v345);
                    if ( v228 )
                    {
LABEL_370:
                      sub_144144330(&v349, L"Commandlet", 0xALL);
                      goto LABEL_386;
                    }
LABEL_385:
                    v217 = 1;
                    byte_14A8DDCAA = 0;
                    LOBYTE(v346) = 1;
                    byte_14A8DDCA9 = 1;
                    byte_14A8DDCA6 = 0;
LABEL_386:
                    if ( !qword_14A6809F0 && !v217 )
                    {
                      sub_140E52110(v351, "InitializeStdOutDevice");
                      sub_140B5A220((__int64)v351);
                    }
                    v230 = 0;
                    if ( (_BYTE)v343 )
                    {
                      v229 = (_DWORD)v350 ? (void *)qword_14A426370(&v349) : &unk_1479D0BD8;
                      if ( !(unsigned int)sub_144179300(v229, L"cookcommandlet") )
                        v230 = 1;
                    }
                    LOBYTE(v343) = v230;
                    sub_140E52110(v351, "IPlatformFeaturesModule::Get()");
                    if ( !qword_14A680A88 )
                    {
                      v231 = sub_1442F64B0();
                      if ( v231 )
                      {
                        v232 = *(_QWORD *)sub_1442E8E80(&v347, v231, 1LL);
                        v233 = sub_1442CB980();
                        qword_14A680A88 = sub_1442CCF00(v233, v232);
                      }
                      else
                      {
                        v234 = (_QWORD *)sub_144020460(8LL);
                        if ( v234 )
                        {
                          *v234 = &off_1479D1920;
                          qword_14A680A88 = (__int64)v234;
                        }
                        else
                        {
                          qword_14A680A88 = 0LL;
                        }
                      }
                    }
                    sub_140B5A220((__int64)v351);
                    sub_140E52110(&v367, "InitGamePhys");
                    if ( !(unsigned __int8)sub_1465E3960() )
                      goto LABEL_465;
                    sub_140B5A220((__int64)&v367);
                    if ( !byte_14A42617C
                      || (v235 = sub_1442516E0(), (unsigned __int8)sub_1442B4720(v235, L"Multiprocess")) )
                    {
LABEL_441:
                      v266 = sub_1442AA870();
                      (*(void (__fastcall **)(__int64, _QWORD))(*(_QWORD *)v266 + 0x60LL))(v266, 0LL);
                      sub_1441FA790();
                      sub_1446E2420(0LL);
                      sub_1462E0310();
                      if ( (unsigned __int8)sub_144257510() )
                      {
                        LOBYTE(v343) = 0;
                        sub_144274A50(
                          qword_14A8D3C78,
                          (unsigned int)L"Audio",
                          (unsigned int)L"UseAudioThread",
                          (unsigned int)&v343,
                          (__int64)&unk_14A8DE090);
                        sub_145FD5420((unsigned __int8)v343);
                      }
                      v267 = sub_144179640();
                      if ( v217 )
                      {
                        if ( v267 )
                        {
                          sub_140E52110(v351, "FPlatformSplash::Show()");
                          sub_1445E7D10();
                          sub_140B5A220((__int64)v351);
                        }
                        sub_140E52110(v351, "FSlateApplication::Create()");
                        sub_1446D50C0();
                        sub_140B5A220((__int64)v351);
                      }
                      else
                      {
                        sub_144598F10();
                        sub_144677B70();
                      }
                      nullsub_1(3LL);
                      v268 = sub_144020460(0x68LL);
                      if ( v268 )
                      {
                        v269 = qword_14A8D3C90;
                        LOBYTE(v3) = v3 | 0x10;
                        v270 = sub_1441AF230(v351, L"Initializing...", L"EngineLoop", L"EngineLoop_Initializing");
                        LOBYTE(v271) = 1;
                        sub_144295820(v268, v272, v270, v271, v269);
                        sub_1442AD650(v268);
                      }
                      else
                      {
                        v268 = 0LL;
                      }
                      v273 = v360;
                      *(_QWORD *)(v360 + 0x80) = v268;
                      if ( (v3 & 0x10) != 0 )
                      {
                        LOBYTE(v3) = v3 & 0xEF;
                        sub_140B65240(v351);
                      }
                      v274 = *(_QWORD *)(v273 + 0x80);
                      v275 = sub_1441D66D0(v351);
                      sub_1442A2FE0(v274, v276, v275);
                      sub_140B65240(v351);
                      *(_QWORD *)&v380 = 0LL;
                      v381 = &off_1479D4CE0;
                      *(_QWORD *)&v379 = sub_140B68200;
                      v277 = sub_144363D40();
                      sub_144367770(v277, &v379);
                      sub_140E52110(v351, "FUniformBufferStruct::InitializeStructs()");
                      sub_14589F9D0();
                      sub_140B5A220((__int64)v351);
                      sub_140E52110(v351, "RHIInit");
                      sub_145814660(0LL);
                      sub_140B5A220((__int64)v351);
                      sub_140E52110(v351, "RenderUtilsInit");
                      sub_145883680();
                      sub_140B5A220((__int64)v351);
                      sub_140E52110(&v374, "FShaderCodeLibrary::InitForRuntime");
                      sub_14587C1A0((unsigned int)dword_14AA4E26C);
                      sub_140B5A220((__int64)&v374);
                      sub_140E52110(v351, "FShaderPipelineCache::Initialize");
                      sub_14589F690((unsigned int)dword_14AA4E26C);
                      sub_140B5A220((__int64)v351);
                      v278 = sub_1442516E0();
                      if ( !(unsigned __int8)sub_1442B4720(v278, L"NoTickRateHandler") )
                      {
                        v279 = sub_144020460(0x120LL);
                        if ( v279 )
                          qword_14AAC05B0 = sub_14685E080(v279);
                        else
                          qword_14AAC05B0 = 0LL;
                      }
                      v280 = sub_144020460(0x1B0LL);
                      if ( v280 )
                        v281 = sub_14655E010(v280);
                      else
                        v281 = 0LL;
                      qword_14AAA60C8 = v281;
                      v282 = sub_1442516E0();
                      v283 = sub_1442B4720(v282, L"NoShaderCompile");
                      sub_140E52110(v351, "GetRendererModule");
                      sub_14620E760();
                      sub_140B5A220((__int64)v351);
                      if ( !v283 )
                      {
                        sub_140E52110(v351, "InitializeShaderTypes");
                        sub_14587E1C0();
                        sub_140B5A220((__int64)v351);
                      }
                      LOBYTE(v284) = 5;
                      sub_1442881A0(v284);
                      v285 = sub_1441D66D0(v351);
                      sub_1442A2FE0(v274, v286, v285);
                      sub_140B65240(v351);
                      if ( !v283 && !v230 )
                      {
                        sub_140E52110(&v367, "CompileGlobalShaderMap");
                        sub_1466EC680(0LL);
                        if ( byte_14A8DDCB2 )
                          goto LABEL_465;
                        sub_140B5A220((__int64)&v367);
                      }
                      sub_140E52110(v351, "CreateMoviePlayer");
                      sub_145E1D090();
                      sub_140B5A220((__int64)v351);
                      if ( (unsigned __int8)sub_145684020() )
                      {
                        sub_140E52110(v351, "FPreLoadScreenManager::Create");
                        sub_145685890();
                        sub_1456874C0();
                        sub_140B5A220((__int64)v351);
                      }
                      sub_140E52110(v351, "PostInitRHI");
                      v345.m128i_i64[0] = 0LL;
                      v345.m128i_i64[1] = 0x48LL;
                      sub_140B77F30(&v345, 0LL);
                      v287 = 0;
                      v288 = &dword_14A454584;
                      v289 = 0LL;
                      do
                      {
                        if ( v289 < 0 || v287 >= v345.m128i_i32[2] )
                          MEMORY[0x10] = 0x5474736172434155LL;
                        v290 = qword_14A426370(&v345);
                        v291 = *v288;
                        ++v287;
                        v288 += 0xA;
                        *(_DWORD *)(v290 + 4 * v289++) = v291;
                      }
                      while ( (__int64)v288 < (__int64)dword_14A4550C4 );
                      sub_1458159C0(&v345);
                      qword_14A426370(&v345);
                      sub_140B69480(&v345, 0LL);
                      v292 = qword_14A426370(&v345);
                      v293 = v346;
                      if ( v292 )
                      {
                        v294 = qword_14A426370(&v345);
                        sub_1441AFDF0(v294);
                      }
                      sub_140B5A220((__int64)v351);
                      if ( byte_14AA60849 )
                      {
                        if ( byte_14AA4E32B )
                        {
                          byte_14AA4E39C = 1;
                          v295 = sub_1442516E0();
                          if ( (unsigned __int8)sub_1442B4720(v295, L"rhithread") )
                          {
                            byte_14AA4E39C = 1;
                          }
                          else
                          {
                            v296 = sub_1442516E0();
                            if ( (unsigned __int8)sub_1442B4720(v296, L"norhithread") )
                              byte_14AA4E39C = 0;
                          }
                        }
                        sub_140E52110(v351, "StartRenderingThread");
                        sub_145885D00();
                        sub_140B5A220((__int64)v351);
                      }
                      nullsub_1(4LL);
                      if ( !byte_14A8DDCA6 )
                      {
                        if ( byte_14AA4E2C8 )
                        {
                          sub_1442CB980();
                          v297 = *(_QWORD *)sub_1442E8D60(&v347, "SlateNullRenderer", 1LL);
                          v298 = sub_1442CB980();
                          v299 = sub_1442CCF00(v298, v297);
                          v300 = v3 | 0x20;
                          v301 = (__int128 *)&v345;
                        }
                        else
                        {
                          sub_1442CB980();
                          v302 = *(_QWORD *)sub_1442E8D60(&v360, "SlateRHIRenderer", 1LL);
                          v303 = sub_1442CB980();
                          v299 = sub_1442CBD50(v303, v302);
                          v300 = v3 | 0x40;
                          v301 = (__int128 *)&v409;
                        }
                        v304 = (__int64 *)(*(__int64 (__fastcall **)(__int64, __int128 *))(*(_QWORD *)v299 + 0x48LL))(
                                            v299,
                                            v301);
                        v305 = (_DWORD *)v304[1];
                        v306 = *v304;
                        v407.m128i_i64[0] = *v304;
                        v407.m128i_i64[1] = (__int64)v305;
                        if ( v305 )
                          ++v305[2];
                        if ( (v300 & 0x40) != 0 )
                        {
                          v307 = (_DWORD *)v409.m128i_i64[1];
                          v300 &= ~0x40u;
                          if ( v409.m128i_i64[1] )
                          {
                            v88 = (*(_DWORD *)(v409.m128i_i64[1] + 8))-- == 1;
                            if ( v88 )
                            {
                              (**(void (__fastcall ***)(_DWORD *))v307)(v307);
                              v88 = v307[3]-- == 1;
                              if ( v88 )
                                (*(void (__fastcall **)(_DWORD *, __int64))(*(_QWORD *)v307 + 8LL))(v307, 1LL);
                            }
                          }
                        }
                        if ( (v300 & 0x20) != 0 )
                        {
                          v308 = (_DWORD *)v345.m128i_i64[1];
                          if ( v345.m128i_i64[1] )
                          {
                            v88 = (*(_DWORD *)(v345.m128i_i64[1] + 8))-- == 1;
                            if ( v88 )
                            {
                              (**(void (__fastcall ***)(_DWORD *))v308)(v308);
                              v88 = v308[3]-- == 1;
                              if ( v88 )
                                (*(void (__fastcall **)(_DWORD *, __int64))(*(_QWORD *)v308 + 8LL))(v308, 1LL);
                            }
                          }
                        }
                        v309 = v407;
                        v345 = v407;
                        if ( v305 )
                          ++v305[2];
                        sub_140E52110(v351, "CurrentSlateApp.InitializeRenderer");
                        v310 = qword_14A915470;
                        v409 = v309;
                        if ( v305 )
                          ++v305[2];
                        (*(void (__fastcall **)(__int64, __m128i *, _QWORD))(*(_QWORD *)v310 + 0x1A0LL))(
                          v310,
                          &v409,
                          0LL);
                        sub_140B5A220((__int64)v351);
                        sub_140E52110(v351, "FEngineFontServices::Create");
                        sub_1468B9060();
                        sub_140B5A220((__int64)v351);
                        sub_140E52110(&v367, "LoadModulesForProject(ELoadingPhase::PostSplashScreen)");
                        v311 = sub_14431B310();
                        if ( !(*(unsigned __int8 (__fastcall **)(__int64, __int64))(*(_QWORD *)v311 + 0x18LL))(
                                v311,
                                2LL)
                          || (v312 = sub_14431B270(),
                              !(*(unsigned __int8 (__fastcall **)(__int64, __int64))(*(_QWORD *)v312 + 0x18LL))(
                                 v312,
                                 2LL)) )
                        {
                          sub_140B5A220((__int64)&v367);
                          if ( v305 )
                          {
                            v88 = v305[2]-- == 1;
                            if ( v88 )
                            {
                              (**(void (__fastcall ***)(_DWORD *))v305)(v305);
                              v88 = v305[3]-- == 1;
                              if ( v88 )
                                (*(void (__fastcall **)(_DWORD *, __int64))(*(_QWORD *)v305 + 8LL))(v305, 1LL);
                            }
                            v88 = v305[2]-- == 1;
                            if ( v88 )
                            {
                              (**(void (__fastcall ***)(_DWORD *))v305)(v305);
                              v88 = v305[3]-- == 1;
                              if ( v88 )
                                (*(void (__fastcall **)(_DWORD *, __int64))(*(_QWORD *)v305 + 8LL))(v305, 1LL);
                            }
                          }
                          v6 = 1;
                          goto LABEL_520;
                        }
                        sub_140B5A220((__int64)&v367);
                        sub_140E52110(&v374, "PlayFirstPreLoadScreen");
                        if ( sub_1456874C0() )
                        {
                          sub_140E52110(v351, "PlayFirstPreLoadScreen - FPreLoadScreenManager::Get()->Initialize");
                          v313 = sub_1456874C0();
                          sub_145688520(v313, v306);
                          sub_140B5A220((__int64)v351);
                          v314 = sub_1456874C0();
                          if ( (unsigned __int8)sub_145688470(v314, 0LL) )
                          {
                            v315 = sub_1456874C0();
                            sub_14568B720(
                              v315,
                              0,
                              v316,
                              v317,
                              (_DWORD)v340,
                              (_DWORD)v342,
                              v343,
                              v344,
                              v345.m128i_i32[0],
                              v345.m128i_i32[2],
                              v346,
                              v347,
                              v348,
                              v349,
                              v350,
                              v351[0],
                              v351[2],
                              v351[4],
                              v352,
                              v353,
                              v354,
                              v357,
                              v358,
                              (_DWORD)v359,
                              v360,
                              v361,
                              v363,
                              v364,
                              v365,
                              v366,
                              v367,
                              v368);
                          }
                        }
                        sub_140B5A220((__int64)&v374);
                        v318 = (char *)(v273 + 0x88);
                        if ( v305 )
                          ++v305[2];
                        if ( &v402 != v318 )
                        {
                          v319 = v309;
                          v309 = *(__m128i *)v318;
                          *(__m128i *)v318 = v319;
                        }
                        v320 = (_DWORD *)_mm_srli_si128(v309, 8).m128i_u64[0];
                        if ( v320 )
                        {
                          v88 = v320[2]-- == 1;
                          if ( v88 )
                          {
                            (**(void (__fastcall ***)(_DWORD *))v320)(v320);
                            v88 = v320[3]-- == 1;
                            if ( v88 )
                              (*(void (__fastcall **)(_DWORD *, __int64))(*(_QWORD *)v320 + 8LL))(v320, 1LL);
                          }
                        }
                        sub_140B65200(&v345);
                        sub_140B65200(&v407);
                      }
                      v345 = 0uLL;
                      sub_140B5A480(&v345, 0xELL);
                      v345.m128i_i32[2] += 0xE;
                      if ( v345.m128i_i32[2] > v345.m128i_i32[3] )
                        sub_140B5A3A0(&v345);
                      v321 = qword_14A426370(&v345);
                      sub_140B5A170(v321, 0xE, (unsigned int)"LauncherToken", 0xE, 0x3F);
                      v322 = sub_146AEADF0();
                      sub_146AEAC50(v322, &v352, &v345);
                      sub_140B5A220((__int64)&v345);
                      sub_140B5A220((__int64)&v352);
                      *(_BYTE *)(v273 + 0x58) = v355;
                      *(_BYTE *)(v273 + 0x59) = v356;
                      *(_BYTE *)(v273 + 0x5A) = BYTE1(v343);
                      *(_BYTE *)(v273 + 0x5F) = BYTE1(v346);
                      *(_WORD *)(v273 + 0x5B) = 0;
                      *(_BYTE *)(v273 + 0x5D) = 0;
                      *(_BYTE *)(v273 + 0x5E) = v293;
                      sub_140B5CE20(v273 + 0x60, &v349);
                      *(_QWORD *)(v273 + 0x70) = v378;
                      *(_QWORD *)(v273 + 0x78) = v372;
                      v6 = 0;
LABEL_520:
                      qword_14A426370(&v369);
                      sub_140B5A380(&v369, 0LL);
                      if ( qword_14A426370(&v369) )
                      {
                        v323 = qword_14A426370(&v369);
                        sub_1441AFDF0(v323);
                      }
                      v324 = qword_14A426370(&v363);
                      v325 = v364;
                      for ( i = v324; v325; --v325 )
                      {
                        qword_14A426370(i);
                        sub_140B5A380(i, 0LL);
                        if ( qword_14A426370(i) )
                        {
                          v327 = qword_14A426370(i);
                          sub_1441AFDF0(v327);
                        }
                        i += 0x10LL;
                      }
                      sub_140B695A0(&v363, 0LL);
                      if ( qword_14A426370(&v363) )
                      {
                        v328 = qword_14A426370(&v363);
                        sub_1441AFDF0(v328);
                      }
                      v329 = qword_14A426370(&v365);
                      v330 = v366;
                      for ( j = v329; v330; --v330 )
                      {
                        qword_14A426370(j);
                        sub_140B5A380(j, 0LL);
                        if ( qword_14A426370(j) )
                        {
                          v332 = qword_14A426370(j);
                          sub_1441AFDF0(v332);
                        }
                        j += 0x10LL;
                      }
                      sub_140B695A0(&v365, 0LL);
                      if ( qword_14A426370(&v365) )
                      {
                        v333 = qword_14A426370(&v365);
                        sub_1441AFDF0(v333);
                      }
                      qword_14A426370(&v349);
                      sub_140B5A380(&v349, 0LL);
                      if ( qword_14A426370(&v349) )
                      {
                        v334 = qword_14A426370(&v349);
                        sub_1441AFDF0(v334);
                      }
                      goto LABEL_536;
                    }
                    sub_140E52110(v351, "FPlatformProcess::CleanShaderWorkingDirectory");
                    v236 = sub_14586DEE0();
                    *(_QWORD *)&v374 = 0x100000000LL;
                    v237 = v236;
                    LODWORD(v375.m128d_f64[0]) = 0xFFFFFFFF;
                    v238 = v236 + 0x10;
                    *(double *)((char *)v375.m128d_f64 + 4) = 0.0;
                    *((_QWORD *)&v374 + 1) = v236 + 0x10;
                    if ( *(_DWORD *)(v236 + 0x28) )
                    {
                      sub_140B6BA10(&v374);
                      v238 = v237 + 0x10;
                    }
                    v239 = v375;
                    v240 = *(_DWORD *)(v237 + 0x28);
                    v241 = v374;
                    LODWORD(v374) = v240 >> 5;
                    DWORD1(v374) = 1 << (v240 & 0x1F);
                    *(_OWORD *)&v389[8] = v241;
                    HIDWORD(v375.m128d_f64[0]) = v240;
                    *(__m128d *)&v389[0x18] = v239;
                    *(_QWORD *)v389 = v237;
                    LODWORD(v375.m128d_f64[0]) = 0xFFFFFFFF << (v240 & 0x1F);
                    v242 = HIDWORD(*(_QWORD *)&v375.m128d_f64[0]);
                    v357 = HIDWORD(*(_QWORD *)&v375.m128d_f64[0]);
                    v379 = *(_OWORD *)v389;
                    LODWORD(v375.m128d_f64[1]) = v240 & 0xFFFFFFE0;
                    v380 = *(_OWORD *)&v389[0x10];
                    v381 = (void **)*(_OWORD *)&_mm_unpackhi_pd(v239, v239);
                    while ( 1 )
                    {
                      v243 = SHIDWORD(v380);
                      if ( HIDWORD(v380) == (_DWORD)v242 && (_QWORD)v380 == v238 && (_QWORD)v379 == v237 )
                      {
                        sub_144166700(v379);
                        sub_140B5A220((__int64)v351);
                        v230 = v343;
                        v217 = v346;
                        LOBYTE(v3) = v354;
                        goto LABEL_441;
                      }
                      v244 = qword_14A426370(v379);
                      v245 = 5 * v243;
                      v246 = (_WORD *)sub_144306440();
                      v347 = 0LL;
                      v247 = 0;
                      v348 = 0LL;
                      v248 = 0;
                      v249 = v246;
                      if ( v246 && *v246 )
                      {
                        v250 = 0xFFFFFFFFFFFFFFFFuLL;
                        do
                          ++v250;
                        while ( v246[v250] );
                        v251 = v250 + 1;
                        if ( v251 > 0 )
                        {
                          sub_140B5A480(&v347, (unsigned int)v251);
                          v248 = HIDWORD(v348);
                          v247 = v348;
                        }
                        LODWORD(v348) = v247 + v251;
                        if ( v247 + v251 > v248 )
                          sub_140B5A3A0(&v347);
                        v252 = qword_14A426370(&v347);
                        sub_14416FA80(v252, v249, 2LL * v251);
                        v247 = v348;
                      }
                      v253 = *(_DWORD *)(v244 + 8 * v245 + 0x18);
                      v254 = v253 - 1;
                      if ( !v253 )
                        v254 = 0;
                      if ( v247 || (v255 = 1, v254 == 0xFFFFFFFF) )
                        v255 = 0;
                      sub_140B5CD80(&v345, &v347, v255 + v254 + 1);
                      v256 = qword_14A426370(v244 + 8 * v245 + 0x10);
                      sub_1441520A0(&v345, v256, v254);
                      qword_14A426370(&v347);
                      sub_140B5A380(&v347, 0LL);
                      if ( qword_14A426370(&v347) )
                      {
                        v257 = qword_14A426370(&v347);
                        sub_1441AFDF0(v257);
                      }
                      v258 = sub_14418C650();
                      if ( v345.m128i_i32[2] )
                        v259 = (void *)qword_14A426370(&v345);
                      else
                        v259 = &unk_1479D0BD8;
                      (*(void (__fastcall **)(__int64, __m128i *, void *))(*(_QWORD *)v258 + 0x100LL))(
                        v258,
                        &v407,
                        v259);
                      sub_14429ABD0(&v345);
                      if ( v345.m128i_i32[2] == v407.m128i_i32[2] )
                      {
                        if ( v345.m128i_i32[2] <= 1 )
                          goto LABEL_439;
                        v261 = qword_14A426370(&v407);
                        v262 = qword_14A426370(&v345);
                        v260 = (unsigned int)sub_144179300(v262, v261) == 0;
                      }
                      else
                      {
                        v260 = v407.m128i_i32[2] + v345.m128i_i32[2] == 1;
                      }
                      if ( !v260 )
                      {
                        v263 = sub_14418C650();
                        if ( v407.m128i_i32[2] )
                          v265 = (void *)qword_14A426370(&v407);
                        else
                          v265 = &unk_1479D0BD8;
                        LOBYTE(v264) = 1;
                        (*(void (__fastcall **)(__int64, void *, _QWORD, __int64))(*(_QWORD *)v263 + 0x60LL))(
                          v263,
                          v265,
                          0LL,
                          v264);
                      }
LABEL_439:
                      sub_140B5A220((__int64)&v407);
                      sub_140B5A220((__int64)&v345);
                      DWORD2(v380) &= ~HIDWORD(v379);
                      sub_140B6BA10((char *)&v379 + 8);
                      LODWORD(v242) = v357;
                      v238 = v237 + 0x10;
                    }
                  }
                }
                else
                {
                  v223 = 0;
                  v224 = 0xBLL;
                }
                sub_140B5A480(&v347, v224);
                goto LABEL_377;
              }
              v97 = qword_14A426370(v94);
              v98 = qword_14A426370(&v349);
              v96 = (unsigned int)sub_144179300(v98, v97) == 0;
            }
            else
            {
              v96 = v95 + (_DWORD)v350 == 1;
            }
            if ( v96 )
              goto LABEL_156;
          }
          v99 = 0;
          goto LABEL_158;
        }
        v89 = qword_14A426370(v86);
        v90 = qword_14A426370(&v369);
        v88 = (unsigned int)sub_144179300(v90, v89) == 0;
      }
      else
      {
        v88 = v87 + v370 == 1;
      }
      v3 = 1;
      v354 = 1;
      if ( v88 )
        goto LABEL_145;
    }
    v91 = 0;
    goto LABEL_147;
  }
  sub_140B5A220((__int64)&v367);
  v6 = 1;
LABEL_536:
  qword_14A426370(&v376);
  sub_140B5A380(&v376, 0LL);
  if ( qword_14A426370(&v376) )
  {
    v335 = qword_14A426370(&v376);
    sub_1441AFDF0(v335);
  }
LABEL_538:
  sub_140B5A220((__int64)v400);
  return v6;
}
