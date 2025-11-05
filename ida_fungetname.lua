

_QWORD *__fastcall sub_14AA6F9E0(_QWORD *a1, __int64 a2)
{
  __int64 v2; // rdi
  unsigned __int64 v4; // rbx
  char v5; // r14
  char *v6; // rsi
  volatile signed __int32 *v7; // r14
  __m128i *v8; // rax
  __m128i v9; // xmm0
  _QWORD *result; // rax
  char v11[4]; // [rsp+20h] [rbp-60h] BYREF
  unsigned int v12; // [rsp+24h] [rbp-5Ch] BYREF
  char v13[13]; // [rsp+28h] [rbp-58h] BYREF
  __int16 v14; // [rsp+35h] [rbp-4Bh]
  char v15; // [rsp+37h] [rbp-49h]
  char v16[16]; // [rsp+38h] [rbp-48h] BYREF
  __int64 v17; // [rsp+48h] [rbp-38h] BYREF
  int v18; // [rsp+50h] [rbp-30h]
  char v19; // [rsp+54h] [rbp-2Ch]
  __int16 v20; // [rsp+55h] [rbp-2Bh]
  char v21; // [rsp+57h] [rbp-29h]
  __m128i v22; // [rsp+58h] [rbp-28h]
  char v23; // [rsp+6Ch] [rbp-14h]

  v2 = a2 + 6;
  v4 = 0xFFFFFFFFFFFFFFFFuLL;
  if ( *(_BYTE *)(a2 + 4) )
  {
    v5 = 1;
    do
      ++v4;
    while ( *(_WORD *)(v2 + 2 * v4) );
  }
  else
  {
    v5 = 0;
    do
      ++v4;
    while ( *(_BYTE *)(v2 + v4) );
  }
  if ( byte_15441686C )
  {
    v6 = (char *)&unk_154416AC0;
  }
  else
  {
    v6 = (char *)sub_14AA70030(&unk_154416AC0);
    byte_15441686C = 1;
  }
  v11[0] = 0;
  v17 = v2;
  v18 = v4;
  v19 = v5;
  v20 = v14;
  v21 = v15;
  if ( v5 )
  {
    v7 = (volatile signed __int32 *)(v6 + 0x10024);
    v8 = (__m128i *)sub_14AA6F160(v16, v2, (unsigned int)v4);
  }
  else
  {
    v7 = (volatile signed __int32 *)(v6 + 0x10020);
    v8 = (__m128i *)sub_14AA6F070(v13, v2, (unsigned int)v4);
  }
  v9 = *v8;
  v23 = 0;
  v22 = v9;
  sub_14AA870B0(&v6[0x40 * _mm_cvtsi128_si32(v9) + 0x10040], &v12, &v17, v11);
  _InterlockedAdd(v7, (unsigned __int8)v11[0]);
  result = a1;
  *a1 = v12;
  return result;
}

__int64 __fastcall sub_140ACA100(__int16 *a1, int a2)
{
  __int16 v2; // ax
  _WORD *v3; // r9
  unsigned int i; // edx
  char v5; // r8^1
  unsigned int v6; // eax
  unsigned int v7; // edx
  unsigned int v8; // edx

  v2 = *a1;
  v3 = a1 + 1;
  for ( i = ~a2; v2; i = dword_1538B0BB0[(unsigned __int8)v8] ^ (v8 >> 8) )
  {
    v5 = HIBYTE(v2);
    ++v3;
    v6 = dword_1538B0BB0[(unsigned __int8)(v2 ^ i)] ^ (i >> 8);
    v7 = dword_1538B0BB0[(unsigned __int8)(v6 ^ v5)] ^ (v6 >> 8);
    v2 = v3[0xFFFFFFFF];
    v8 = dword_1538B0BB0[(unsigned __int8)v7] ^ (v7 >> 8);
  }
  return ~i;
}

__int64 __fastcall sub_14AA8D5F0(__int64 a1, __int64 a2)
{
  unsigned int v3; // r14d
  __int64 v5; // rax
  __int64 v6; // rbx
  int v7; // r8d
  __int64 i; // rdi
  _WORD *v9; // rcx
  __int64 v10; // rax
  __int128 v12; // [rsp+20h] [rbp-18h] BYREF

  v3 = *(_DWORD *)(a1 + 0xC);
  v5 = *(_QWORD *)(a1 + 0x10);
  v6 = v3 & *(_DWORD *)(a2 + 0x14);
  v7 = *(_DWORD *)(v5 + 4 * v6);
  for ( i = v5 + 4 * v6; v7; i = v10 + 4 * v6 )
  {
    if ( (v7 & 0x80000000) == *(_DWORD *)(a2 + 0x18) )
    {
      v9 = (_WORD *)(*(_QWORD *)(*(_QWORD *)(a1 + 0x18) + 8 * ((unsigned __int64)(v7 & 0x7FFFFFFF) >> 0x12) + 8)
                   + 2 * (v7 & 0x3FFFFu));
      if ( *v9 == *(_WORD *)(a2 + 0x1C) )
      {
        v12 = *(_OWORD *)a2;
        if ( (unsigned __int8)sub_14AA6EC60(v9, &v12) )
          break;
      }
    }
    v10 = *(_QWORD *)(a1 + 0x10);
    v6 = v3 & ((_DWORD)v6 + 1);
    v7 = *(_DWORD *)(v10 + 4 * v6);
  }
  return i;
}
__int64 __fastcall sub_14AA6F070(__int64 a1, __int64 a2, __int64 a3)
{
  int v3; // edi
  int v4; // ebx
  __int64 v6; // r9
  __int64 v7; // rdx
  int *v8; // r10
  __int64 v9; // rax
  int v11[256]; // [rsp+20h] [rbp-418h] BYREF

  v3 = 0;
  v4 = a3;
  v6 = 0LL;
  if ( (_DWORD)a3 )
  {
    v7 = a2 - (_QWORD)v11;
    v8 = v11;
    do
    {
      if ( (unsigned int)v6 >= 0x400 )
        break;
      v6 = (unsigned int)(v6 + 1);
      *(_BYTE *)v8 = *((_BYTE *)v8 + v7) + ((unsigned int)(*((char *)v8 + v7) - 0x41) < 0x1A ? 0x20 : 0);
      v8 = (int *)((char *)v8 + 1);
    }
    while ( (unsigned int)v6 < (unsigned int)a3 );
  }
  v9 = sub_14A946300(v11, (unsigned int)a3, a3, v6);
  if ( v4 == 4 )
    LOBYTE(v3) = (v11[0] & 0xDFDFDFDF) == 0x454E4F4E;
  *(_DWORD *)(a1 + 4) = v9;
  *(_DWORD *)a1 = BYTE4(v9) & 0xF;
  *(_DWORD *)(a1 + 8) = HIDWORD(v9) & 0x80000000 | (v3 << 0x1F);
  *(_WORD *)(a1 + 0xC) = 2 * ((0x20 * v4) | (HIDWORD(v9) >> 4) & 0x1F);
  return a1;
}__int64 __fastcall sub_14AA6F160(__int64 a1, __int64 a2, unsigned int a3, char *a4)

_int64 __fastcall sub_()
{
  unsigned int v5; // eax
  __int64 v6; // r8
  __int64 v8; // rax
  int v9; // edx
  __int64 result; // rax
  __int16 v11; // bx
  char v12[2048]; // [rsp+20h] [rbp-818h] BYREF

  v5 = 0;
  v6 = a2;
  if ( a3 )
  {
    v6 = a2 - (_QWORD)v12;
    a4 = v12;
    do
    {
      if ( v5 >= 0x400 )
        break;
      ++v5;
      *(_WORD *)a4 = *(_WORD *)&a4[v6] + ((unsigned int)*(unsigned __int16 *)&a4[v6] - 0x41 < 0x1A ? 0x20 : 0);
      a4 += 2;
    }
    while ( v5 < a3 );
  }
  v8 = sub_14A946300(v12, 2 * a3, v6, a4);
  *(_DWORD *)(a1 + 4) = v8;
  *(_DWORD *)a1 = BYTE4(v8) & 0xF;
  v9 = HIDWORD(v8) >> 4;
  *(_DWORD *)(a1 + 8) = HIDWORD(v8) & 0x80000000;
  result = a1;
  v11 = *(_WORD *)(a1 + 0xC) & 0x3E | ((_WORD)a3 << 6) | 1;
  *(_WORD *)(a1 + 0xC) = v11 ^ ((unsigned __int8)v11 ^ (unsigned __int8)(2 * v9)) & 0x3E;
  return result;
}

