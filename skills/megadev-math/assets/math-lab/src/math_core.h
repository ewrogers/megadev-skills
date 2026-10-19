#ifndef MATH_CORE_H
#define MATH_CORE_H
/* Application helpers. Signed Q8.8 means raw/256, range -128..127.99609375. */
typedef signed short MmS16;
typedef unsigned short MmU16;
typedef signed int MmS32;
typedef unsigned int MmU32;
_Static_assert(sizeof(MmS16)==2 && sizeof(MmS32)==4,"Math requires 16/32-bit integers");
#define MM_ONE 256
#define MM_TURN 1024
typedef struct {MmS16 x,y;} MmVec;
#if defined(_WIN32) && defined(MATH_HOST_DLL)
#define MM_API __declspec(dllexport)
#else
#define MM_API
#endif
/* Checked results preserve *out on failure. Null output is a failure. */
MM_API MmU16 mm_from_int(MmS16 value,MmS16 *out);
/* Conversions and rescaling truncate toward zero. */
MM_API MmS16 mm_to_int(MmS16 value);
MM_API MmU16 mm_add(MmS16 a,MmS16 b,MmS16 *out);
MM_API MmU16 mm_sub(MmS16 a,MmS16 b,MmS16 *out);
MM_API MmU16 mm_mul(MmS16 a,MmS16 b,MmS16 *out);
MM_API MmU16 mm_div(MmS16 a,MmS16 b,MmS16 *out);
MM_API MmU16 mm_isqrt(MmU32 value);
/* Non-null input required. Raw unsigned Q16.16, not a Q8.8 distance. */
MM_API MmU32 mm_length2(MmVec const *v);
MM_API MmU16 mm_normalize(MmVec const *v,MmVec *out);
/* Raw signed Q16.16; an overflowing sum fails without writing. */
MM_API MmU16 mm_dot(MmVec const *a,MmVec const *b,MmS32 *out);
/* Angle wraps modulo 1024; returned sine/cosine are Q8.8. */
MM_API MmS16 mm_sin(MmU16 angle);
MM_API MmS16 mm_cos(MmU16 angle);
MM_API MmU16 mm_rotate(MmVec const *v,MmU16 angle,MmVec *out);
/* Progress 0..256 inclusive; invalid progress fails. */
MM_API MmU16 mm_lerp(MmS16 a,MmS16 b,MmU16 t,MmS16 *out);
MM_API MmU16 mm_smoothstep(MmU16 t,MmU16 *out);
#endif
