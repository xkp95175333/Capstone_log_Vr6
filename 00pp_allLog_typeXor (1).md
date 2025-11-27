```cpp
โอเค ผมตอบแบบ ชัด 100% ตามสิ่งที่คุณมีใน Pipeline ตอนนี้
คุณถามว่า:

> “มีกี่อันหรอ?”
(หมายถึง มี decoder/checker ทั้งหมดกี่แบบแล้ว?)



นี่คือจำนวน “จริงทั้งหมด” ที่เราใช้ในระบบ Pointer Decode Master


---

⭐ จำนวนทั้งหมด = 12 อัน

แบ่งเป็น 3 กลุ่ม


---

🟦 กลุ่ม A — Byte/Nibble Decode (6 อัน)

ลำดับ	ฟังก์ชัน	อธิบาย

A1	TrimTrailingNibbleEp2	ตัด nibble ท้ายแบบ key 1 nibble
A2	TrimTrailingNibble	ตัด nibble แบบ X000
A3	DecodeStrip00	เอา 00 ที่แทรกออก
A4	SwapNibbles	สลับ nibble ครึ่ง byte
A5	Swap64	reverse endian 64-bit
A6	ReverseBits	กลับบิตทั้งหมด


✔ รวมกลุ่ม A = 6 อัน


---

🟩 กลุ่ม B — Bitfield 9-bit Reassemble (3 อัน)

ลำดับ	ฟังก์ชัน	อธิบาย

B1	ExtractBlocks9	แยก pointer เป็น 4 block × 9 bit
B2	MergeBlocks9	รวม block กลับตามลำดับ
B3	TryDecodeBlocks9	จัด perm 24 แบบเพื่อคืน pointer จริง


✔ รวมกลุ่ม B = 3 อัน


---

🟧 กลุ่ม C — Combined Multi-Stage Decode (3 อัน)

ลำดับ	ฟังก์ชัน	อธิบาย

C1	TryDecodeBlocks9(SwapNibbles)	สลับ nibble แล้ว decode block
C2	TryDecodeBlocks9(ReverseBits)	reverse bits + block decode
C3	MasterCombine (ใน MasterDecode)	รวมทุกแบบและให้คะแนน


✔ รวมกลุ่ม C = 3 อัน


---

🎉 สรุปทั้งหมด

กลุ่ม	จำนวน

A	6
B	3
C	3
รวมทั้งหมด	⭐ 12 อัน ⭐



---

🔥 ถ้ารวม Checker + Scoring + Validator

จะมีเพิ่ม:

🟡 Validator Struct (3 อัน)

ValidFVector

ValidFQuat

ValidFTransform


🟠 Encrypt / Pointer Checker (2 อัน)

IsEncryptedTransform

IsUEOffsetGroup


🟣 Reader / Scanner (2 อัน)

read128Into

ScoreReadStruct



---

✔ ถ้านับทั้งหมดจริง ๆ ที่ใช้งาน

12 Decode + 7 Validator/Checker = 19 ฟังก์ชัน


---

🔥 ถ้านับทุกอย่างในไฟล์ PointerDecodeMaster.hpp ที่สร้างให้คุณ

รวมทั้งหมด:

ฟังก์ชันทั้งหมด = 27 อัน

เพราะมี:

Decoder 12

Validator 7

Scoring 3

Struct Read 2

MasterDecode 3



---

✔ คำตอบสุดท้าย (ตามที่คุณถามว่า “มีกี่อัน?”)

การ decode pointer = 12 อัน (A+B+C)

รวมตัวเช็คทั้งหมด = 27 ฟังก์ชัน


---

ถ้าต้องการ “รายการเป็นตัวเลข 1–27 พร้อมชื่อ” บอกได้เลย เดี๋ยวผมพิมพ์ให้เป็นรายการพร้อม Copy&Paste ❤️
┌───────────────────────────────────────────────┐
│                FEncTransform (0x40)           │
├─────────────┬─────────────────────────────────┤
│ 0x00 - 0x0F │ Rotation : FQuat (float4)        │
│             │   x, y, z, w                     │
├─────────────┼─────────────────────────────────┤
│ 0x10 - 0x1B │ Translation : FVector (float3)   │
│             │   X, Y, Z                        │
├─────────────┼─────────────────────────────────┤
│ 0x1C - 0x1F │ Padding (4 bytes for alignment)  │
├─────────────┼─────────────────────────────────┤
│ 0x20 - 0x2B │ Scale3D : FVector (float3)       │
│             │   X, Y, Z                        │
├─────────────┼─────────────────────────────────┤
│ 0x2C - 0x2F │ Padding (alignment)              │
├─────────────┼─────────────────────────────────┤
│ 0x30 - 0x33 │ EncHandler : FEncHandler (4 bytes)  │
│             │   uint16  index                      │
│             │   int8    bEncrypted   ◄── จุดสำคัญ │
│             │   uint8   bDynamic                    │
├─────────────┼─────────────────────────────────┤
│ 0x34 - 0x3F │ Padding / Reserved (12 bytes)     │
│             │   (ไม่ใช่ key, ไม่ใช่ข้อมูลสำคัญ)│
└─────────────┴─────────────────────────────────┘
Total = 64 bytes (0x40)

FTransform (0x30)                 |       FEncTransform (0x40)
──────────────────────────────────────┼────────────────────────────────────────
0x00  [FQuat Rotation] (16B)          |  0x00 [FQuat Rotation] (16B)
0x10  [FVector Translation] (12B)     |  0x10 [FVector Translation] (12B)
0x1C  [pad] (4B)                      |  0x1C [pad] (4B)
0x20  [FVector Scale3D] (12B)         |  0x20 [FVector Scale3D] (12B)
0x2C  [pad] (4B)                      |  0x2C [pad] (4B)
                                      |  0x30 [FEncHandler] (4B)
                                      |  0x34 [Padding] (12B)
──────────────────────────────────────┴────────────────────────────────────────
           SIZE = 48 bytes (0x30)     |      SIZE = 64 bytes (0x40)

┌──────────────────────────────────────────────────┐
│                 FEncTransform (0x40)              │
├─────────────────────────┬────────────────────────┤
│ Offset 0x00 (16 bytes)  │    FQuat Rotation       │
│                         │   x, y, z, w (float4)   │
│                         │   (*ยังคงมีค่าสมบูรณ์) │
├─────────────────────────┼────────────────────────┤
│ Offset 0x10 (12 bytes)  │  FVector Translation     │
│                         │       X, Y, Z            │
│ Offset 0x1C (4 bytes)   │      Padding (align)     │
├─────────────────────────┼────────────────────────┤
│ Offset 0x20 (12 bytes)  │    FVector Scale3D       │
│                         │       X, Y, Z            │
│ Offset 0x2C (4 bytes)   │      Padding (align)     │
├─────────────────────────┼────────────────────────┤
│ Offset 0x30 (4 bytes)   │   FEncHandler EncHandler │
│                         │   ┌───────────────┐      │
│                         │   │ index (uint16)│      │
│                         │   │ bEncrypted(i8)│◄─── used to check ENCRYPTED
│                         │   │ bDynamic(u8)  │      │
│                         │   └───────────────┘      │
├─────────────────────────┼────────────────────────┤
│ Offset 0x34 (12 bytes)  │   Padding / Reserved     │
│                         │   (not used for bones)   │
└─────────────────────────┴────────────────────────┘
TOTAL SIZE = **0x40**



```
