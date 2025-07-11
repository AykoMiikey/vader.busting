#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
// Parameters:
//
//   sampler2D texSampler;
//   float texelWidth;
//
//
// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   texelWidth   c0       1
//   texSampler   s0       1
//

    ps_2_0
    def c1, 1.38461542, 0.31621623, 0.227027029, 3.23076916
    def c2, 0.0702702701, 0, 0, 0
    dcl t0.xy
    dcl_2d s0
    mov r0.xw, c1
    mad r1.x, c0.x, r0.x, t0.x
    mov r1.y, t0.y
    mad r0.x, c0.x, -r0.x, t0.x
    mov r0.y, t0.y
    mad r2.x, c0.x, -r0.w, t0.x
    mov r2.y, t0.y
    mad r3.x, c0.x, r0.w, t0.x
    mov r3.y, t0.y
    texld r1, r1, s0
    texld r0, r0, s0
    texld r4, t0, s0
    texld r2, r2, s0
    texld r3, r3, s0
    mul r0.xyz, r0, c1.y
    mad r0.xyz, r4, c1.z, r0
    mad r0.xyz, r1, c1.y, r0
    mad r0.xyz, r2, c2.x, r0
    mad r4.xyz, r3, c2.x, r0
    mov oC0, r4

// approximately 20 instruction slots used (5 texture, 15 arithmetic)
#endif

#include <Windows.h>

const BYTE blur_x[] =
{
      0,   2, 255, 255, 254, 255, 
     44,   0,  67,  84,  65,  66, 
     28,   0,   0,   0, 131,   0, 
      0,   0,   0,   2, 255, 255, 
      2,   0,   0,   0,  28,   0, 
      0,   0,   0,   1,   0,   0, 
    124,   0,   0,   0,  68,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  80,   0, 
      0,   0,   0,   0,   0,   0, 
     96,   0,   0,   0,   2,   0, 
      0,   0,   1,   0,   0,   0, 
    108,   0,   0,   0,   0,   0, 
      0,   0, 116, 101, 120,  83, 
     97, 109, 112, 108, 101, 114, 
      0, 171,   4,   0,  12,   0, 
      1,   0,   1,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
    116, 101, 120, 101, 108,  87, 
    105, 100, 116, 104,   0, 171, 
      0,   0,   3,   0,   1,   0, 
      1,   0,   1,   0,   0,   0, 
      0,   0,   0,   0, 112, 115, 
     95,  50,  95,  48,   0,  77, 
    105,  99, 114, 111, 115, 111, 
    102, 116,  32,  40,  82,  41, 
     32,  72,  76,  83,  76,  32, 
     83, 104,  97, 100, 101, 114, 
     32,  67, 111, 109, 112, 105, 
    108, 101, 114,  32,  49,  48, 
     46,  49,   0, 171,  81,   0, 
      0,   5,   1,   0,  15, 160, 
     20,  59, 177,  63,  24, 231, 
    161,  62, 198, 121, 104,  62, 
    236, 196,  78,  64,  81,   0, 
      0,   5,   2,   0,  15, 160, 
    220, 233, 143,  61,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  31,   0, 
      0,   2,   0,   0,   0, 128, 
      0,   0,   3, 176,  31,   0, 
      0,   2,   0,   0,   0, 144, 
      0,   8,  15, 160,   1,   0, 
      0,   2,   0,   0,   9, 128, 
      1,   0, 228, 160,   4,   0, 
      0,   4,   1,   0,   1, 128, 
      0,   0,   0, 160,   0,   0, 
      0, 128,   0,   0,   0, 176, 
      1,   0,   0,   2,   1,   0, 
      2, 128,   0,   0,  85, 176, 
      4,   0,   0,   4,   0,   0, 
      1, 128,   0,   0,   0, 160, 
      0,   0,   0, 129,   0,   0, 
      0, 176,   1,   0,   0,   2, 
      0,   0,   2, 128,   0,   0, 
     85, 176,   4,   0,   0,   4, 
      2,   0,   1, 128,   0,   0, 
      0, 160,   0,   0, 255, 129, 
      0,   0,   0, 176,   1,   0, 
      0,   2,   2,   0,   2, 128, 
      0,   0,  85, 176,   4,   0, 
      0,   4,   3,   0,   1, 128, 
      0,   0,   0, 160,   0,   0, 
    255, 128,   0,   0,   0, 176, 
      1,   0,   0,   2,   3,   0, 
      2, 128,   0,   0,  85, 176, 
     66,   0,   0,   3,   1,   0, 
     15, 128,   1,   0, 228, 128, 
      0,   8, 228, 160,  66,   0, 
      0,   3,   0,   0,  15, 128, 
      0,   0, 228, 128,   0,   8, 
    228, 160,  66,   0,   0,   3, 
      4,   0,  15, 128,   0,   0, 
    228, 176,   0,   8, 228, 160, 
     66,   0,   0,   3,   2,   0, 
     15, 128,   2,   0, 228, 128, 
      0,   8, 228, 160,  66,   0, 
      0,   3,   3,   0,  15, 128, 
      3,   0, 228, 128,   0,   8, 
    228, 160,   5,   0,   0,   3, 
      0,   0,   7, 128,   0,   0, 
    228, 128,   1,   0,  85, 160, 
      4,   0,   0,   4,   0,   0, 
      7, 128,   4,   0, 228, 128, 
      1,   0, 170, 160,   0,   0, 
    228, 128,   4,   0,   0,   4, 
      0,   0,   7, 128,   1,   0, 
    228, 128,   1,   0,  85, 160, 
      0,   0, 228, 128,   4,   0, 
      0,   4,   0,   0,   7, 128, 
      2,   0, 228, 128,   2,   0, 
      0, 160,   0,   0, 228, 128, 
      4,   0,   0,   4,   4,   0, 
      7, 128,   3,   0, 228, 128, 
      2,   0,   0, 160,   0,   0, 
    228, 128,   1,   0,   0,   2, 
      0,   8,  15, 128,   4,   0, 
    228, 128, 255, 255,   0,   0
};
