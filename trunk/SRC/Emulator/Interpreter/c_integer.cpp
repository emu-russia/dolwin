// Integer Instructions
#include "dolphin.h"

#define OP(name) void __fastcall c_##name##(u32 op)

#define COMPUTE_CR0(r)                                                                \
{                                                                                     \
    (CR = (CR & 0xfffffff)                   |                                        \
    ((XER & (1 << 31)) ? (0x10000000) : (0)) |                                        \
    (((s32)(r) < 0) ? (0x80000000) : (((s32)(r) > 0) ? (0x40000000) : (0x20000000))));\
}

#define SET_XER_SO      (XER |=  (1 << 31))
#define SET_XER_OV      (XER |=  (1 << 30))
#define SET_XER_CA      (XER |=  (1 << 29))
#define RESET_XER_SO    (XER &= ~(1 << 31))
#define RESET_XER_OV    (XER &= ~(1 << 30))
#define RESET_XER_CA    (XER &= ~(1 << 29))
#define IS_XER_CA       (XER & (1 << 29))

// rd = (ra | 0) + SIMM
OP(ADDI)
{
    if(RA) RRD = RRA + SIMM;
    else RRD = SIMM;
}

// rd = (ra | 0) + (SIMM || 0x0000)
OP(ADDIS)
{
    if(RA) RRD = RRA + (SIMM << 16);
    else RRD = SIMM << 16;
}

// rd = ra + rb
OP(ADD)
{
    RRD = RRA + RRB;
}

// rd = ra + rb, CR0
OP(ADDD)
{
    u32 res = RRA + RRB;
    RRD = res;
    COMPUTE_CR0(res);
}

// rd = ra + rb, XER
OP(ADDO)
{
    u32 a = RRA, b = RRB, res;
    BOOL ovf = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jno     no_ovf
    __asm   mov     ovf, 1
no_ovf:
    __asm   mov     res, eax

    RRD = res;
    if(ovf)
    {
        SET_XER_OV;
        SET_XER_SO;
    }
    else RESET_XER_OV;
}

// rd = ra + rb, CR0, XER
OP(ADDOD)
{
    u32 a = RRA, b = RRB, res;
    BOOL ovf = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jno     no_ovf
    __asm   mov     ovf, 1
no_ovf:
    __asm   mov     res, eax

    RRD = res;
    if(ovf)
    {
        SET_XER_OV;
        SET_XER_SO;
    }
    else RESET_XER_OV;
    COMPUTE_CR0(res);
}

// rd = ~ra + rb + 1
OP(SUBF)
{
    RRD = ~RRA + RRB + 1;
}

// rd = ~ra + rb + 1, CR0
OP(SUBFD)
{
    u32 res = ~RRA + RRB + 1;
    RRD = res;    
    COMPUTE_CR0(res);
}

// rd = ~ra + rb + 1, XER
OP(SUBFO)
{
}

// rd = ~ra + rb + 1, CR0, XER
OP(SUBFOD)
{
}

// rd = ra + SIMM, XER
OP(ADDIC)
{
    u32 a = RRA, b = SIMM, res;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
}

// rd = ra + SIMM, CR0, XER
OP(ADDICD)
{
    u32 a = RRA, b = SIMM, res;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
    COMPUTE_CR0(res);
}

// rd = ~RRA + SIMM + 1, XER
OP(SUBFIC)
{
    u32 a = ~RRA, b = SIMM + 1, res;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
}

// rd = ra + rb, XER[CA]
OP(ADDC)
{
    u32 a = RRA, b = RRB, res;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
}

// rd = ra + rb, XER[CA], CR0
OP(ADDCD)
{
    u32 a = RRA, b = RRB, res;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
    COMPUTE_CR0(res);
}

// rd = ra + rb, XER[CA], XER[OV]
OP(ADDCO)
{
    u32 a = RRA, b = RRB, res;
    BOOL carry = FALSE, ovf = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   jno     no_ovf
    __asm   mov     ovf, 1
no_ovf:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
    if(ovf)
    {
        SET_XER_OV;
        SET_XER_SO;
    }
    else RESET_XER_OV;
}

// rd = ra + rb, XER[CA], XER[OV], CR0
OP(ADDCOD)
{
    u32 a = RRA, b = RRB, res;
    BOOL carry = FALSE, ovf = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   jno     no_ovf
    __asm   mov     ovf, 1
no_ovf:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
    if(ovf)
    {
        SET_XER_OV;
        SET_XER_SO;
    }
    else RESET_XER_OV;
    COMPUTE_CR0(res);
}

// rd = ~ra + rb + 1, XER[CA]
OP(SUBFC)
{
    u32 a = ~RRA, b = RRB + 1, res;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    if(carry) SET_XER_CA; else RESET_XER_CA;
    RRD = res;
}

// rd = ~ra + rb + 1, XER[CA], CR0
OP(SUBFCD)
{
    u32 a = ~RRA, b = RRB + 1, res;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    if(carry) SET_XER_CA; else RESET_XER_CA;
    RRD = res;
    COMPUTE_CR0(res);
}

// ---------------------------------------------------------------------------

static void ADDXER(u32 a, u32 op)
{
    u32 res;
    u32 c = (IS_XER_CA) ? 1 : 0;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, c
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
}

static void ADDXERD(u32 a, u32 op)
{
    u32 res;
    u32 c = (IS_XER_CA) ? 1 : 0;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, c
    __asm   add     eax, ecx
    __asm   jnc     no_carry
    __asm   mov     carry, 1
no_carry:
    __asm   mov     res, eax

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
    COMPUTE_CR0(res);
}

static void ADDXER2(u32 a, u32 b, u32 op)
{
    u32 res;
    u32 c = (IS_XER_CA) ? 1 : 0;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   xor     edx, edx    // upper 32 bits of 64-bit operand

    __asm   add     eax, ecx
    __asm   adc     edx, edx

    __asm   add     eax, c
    __asm   adc     edx, 0

    __asm   mov     res, eax
    __asm   mov     carry, edx  // now save carry

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
}

static void ADDXER2D(u32 a, u32 b, u32 op)
{
    u32 res;
    u32 c = (IS_XER_CA) ? 1 : 0;
    BOOL carry = FALSE;

    __asm   mov     eax, a
    __asm   mov     ecx, b
    __asm   xor     edx, edx    // upper 32 bits of 64-bit operand

    __asm   add     eax, ecx
    __asm   adc     edx, edx

    __asm   add     eax, c
    __asm   adc     edx, 0

    __asm   mov     res, eax
    __asm   mov     carry, edx  // now save carry

    RRD = res;
    if(carry) SET_XER_CA; else RESET_XER_CA;
    COMPUTE_CR0(res);
}

// rd = ra + rb + XER[CA], XER
OP(ADDE)
{
    ADDXER2(RRA, RRB, op);
}

// rd = ra + rb + XER[CA], CR0, XER
OP(ADDED)
{
    ADDXER2D(RRA, RRB, op);
}

// rd = ~ra + rb + XER[CA], XER
OP(SUBFE)
{
    ADDXER2(~RRA, RRB, op);
}

// rd = ~ra + rb + XER[CA], CR0, XER
OP(SUBFED)
{
    ADDXER2D(~RRA, RRB, op);
}

// rd = ra + XER[CA] - 1 (0xffffffff), XER
OP(ADDME)
{
    ADDXER(RRA - 1, op);
}

// rd = ra + XER[CA] - 1 (0xffffffff), CR0, XER
OP(ADDMED)
{
    ADDXERD(RRA - 1, op);
}

// rd = ~ra + XER[CA] - 1, XER
OP(SUBFME)
{
    ADDXER(~RRA - 1, op);
}

// rd = ~ra + XER[CA] - 1, CR0, XER
OP(SUBFMED)
{
    ADDXERD(~RRA - 1, op);
}

// rd = ra + XER[CA], XER
OP(ADDZE)
{
    ADDXER(RRA, op);
}

// rd = ra + XER[CA], CR0, XER
OP(ADDZED)
{
    ADDXERD(RRA, op);
}

// rd = ~ra + XER[CA], XER
OP(SUBFZE)
{
    ADDXER(~RRA, op);
}

// rd = ~ra + XER[CA], CR0, XER
OP(SUBFZED)
{
    ADDXERD(~RRA, op);
}

// ---------------------------------------------------------------------------

// rd = ~ra + 1
OP(NEG)
{
    RRD = ~RRA + 1;
}

// rd = ~ra + 1, CR0
OP(NEGD)
{
    u32 res = ~RRA + 1;
    RRD = res;
    COMPUTE_CR0(res);
}

// ---------------------------------------------------------------------------

// prod[0-48] = ra * SIMM
// rd = prod[16-48]
OP(MULLI)
{
    RRD = RRA * SIMM;
}

// prod[0-48] = ra * rb
// rd = prod[16-48]
OP(MULLW)
{
    s32 a = RRA, b = RRB;
    s64 res = a * b;
    RRD = (s32)(res & 0x00000000ffffffff);
}

// prod[0-48] = ra * rb
// rd = prod[16-48]
// CR0
OP(MULLWD)
{
    s32 a = RRA, b = RRB;
    s64 res = a * b;
    RRD = (s32)(res & 0x00000000ffffffff);
    COMPUTE_CR0(res);
}

// prod[0-63] = ra * rb
// rd = prod[0-31]
OP(MULHW)
{
    s64 a = (s32)RRA, b = (s32)RRB, res = a * b;
    res = (res >> 32);
    RRD = (s32)res;
}

// prod[0-63] = ra * rb
// rd = prod[0-31]
// CR0
OP(MULHWD)
{
    s64 a = (s32)RRA, b = (s32)RRB, res = a * b;
    res = (res >> 32);
    RRD = (s32)res;
    COMPUTE_CR0(res);
}

// prod[0-63] = ra * rb
// rd = prod[0-31]
OP(MULHWU)
{
    u64 a = RRA, b = RRB, res = a * b;
    res = (res >> 32);
    RRD = (u32)res;
}

// prod[0-63] = ra * rb
// rd = prod[0-31]
// CR0
OP(MULHWUD)
{
    u64 a = RRA, b = RRB, res = a * b;
    res = (res >> 32);
    RRD = (u32)res;
    COMPUTE_CR0(res);
}

// rd = ra / rb (signed)
OP(DIVW)
{
    s32 a = RRA, b = RRB;
    if(b) RRD = a / b;
}

// rd = ra / rb (signed), CR0
OP(DIVWD)
{
    s32 a = RRA, b = RRB, res;
    if(b)
    {
        res = a / b;
        RRD = res;
        COMPUTE_CR0(res);
    }
}

// rd = ra / rb (unsigned)
OP(DIVWU)
{
    u32 a = RRA, b = RRB, res = 0;
    if(b) RRD = a / b;
}

// rd = ra / rb (unsigned), CR0
OP(DIVWUD)
{
    u32 a = RRA, b = RRB, res;
    if(b)
    {
        res = a / b;
        RRD = res;
        COMPUTE_CR0(res);
    }
}