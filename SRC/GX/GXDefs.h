// All GX architectural definitions (register names, bit names, etc.)

// More GX info: https://github.com/ogamespec/dolwin-docs/blob/master/HW/GraphicsSystem/GX.md

// TODO: Partially the definitions were dragged from DolwinVideo, need to rename properly and rearrange to enum class instead of #defines.

#pragma once

namespace GX
{
	// CP Commands. Format of commands transmitted via FIFO and display lists (DL)

	enum class CPCommand
	{
		CP_CMD_NOP = 0x00,					// 00000000
		CP_CMD_VCACHE_INVD = 0x48,			// 01001xxx
		CP_CMD_CALL_DL = 0x40,				// 01000xxx
		CP_CMD_LOAD_BPREG = 0x60,			// 0110,SUattr(3:0), Address[7:0], 24 bits data
		CP_CMD_LOAD_CPREG = 0x08,			// 00001xxx, Address[7:0], 32 bits data
		CP_CMD_LOAD_XFREG = 0x10,			// 00010xxx
		CP_CMD_LOAD_INDXA = 0x20,			// 00100xxx
		CP_CMD_LOAD_INDXB = 0x28,			// 00101xxx
		CP_CMD_LOAD_INDXC = 0x30,			// 00110xxx
		CP_CMD_LOAD_INDXD = 0x38,			// 00111xxx
		CP_CMD_DRAW_QUAD = 0x80,			// 10000,vat(2:0)
		CP_CMD_DRAW_TRIANGLE = 0x90,		// 10010,vat(2:0)
		CP_CMD_DRAW_STRIP = 0x98,			// 10011,vat(2:0)
		CP_CMD_DRAW_FAN = 0xA0,				// 10100,vat(2:0)
		CP_CMD_DRAW_LINE = 0xA8,			// 10101,vat(2:0)
		CP_CMD_DRAW_LINESTRIP = 0xB0,		// 10110,vat(2:0)
		CP_CMD_DRAW_POINT = 0xB8,			// 10111,vat(2:0)
	};

	// CP Registers (from CPU side). 16-bit access

	// CP Registers (from GX side). These registers are available only for writing, with the CP_LoadRegs command

	enum class CPRegister
	{
		CP_VC_STAT_RESET_ID = 0x00,
		CP_STAT_ENABLE_ID = 0x10,
		CP_STAT_SEL_ID = 0x20,
		CP_MATINDEX_A_ID = 0x30,			// MatrixIndexA 0011xxxx 
		CP_MATINDEX_B_ID = 0x40,			// MatrixIndexB 0100xxxx 
		CP_VCD_LO_ID = 0x50,				// VCD_Lo 0101xxxx
		CP_VCD_HI_ID = 0x60,				// VCD_Hi 0110xxxx
		CP_VAT_A_ID = 0x70,					// VAT_group0 0111x,vat[2:0]
		CP_VAT_B_ID = 0x80,					// VAT_group1 1000x,vat[2:0]
		CP_VAT_C_ID = 0x90,					// VAT_group2 1001x,vat[2:0]
		CP_ARRAY_BASE_ID = 0xa0,			// ArrayBase 1001,array[3:0]
		CP_ARRAY_STRIDE_ID = 0xb0,			// ArrayStride 1011,array[3:0]
	};

	// XF Registers

	// BP (ByPass) address space (SU/TEV etc) Registers

	#define GEN_MODE						0x00

	#define BU_IMASK						0x0f

	#define SU_SCIS0						0x20
	#define SU_SCIS1						0x21
	#define SU_LPSIZE						0x22

	#define RAS1_TREF0						0x28
	#define RAS1_TREF1						0x29
	#define RAS1_TREF2						0x2A
	#define RAS1_TREF3						0x2B
	#define RAS1_TREF4						0x2C
	#define RAS1_TREF5						0x2D
	#define RAS1_TREF6						0x2E
	#define RAS1_TREF7						0x2F

	#define SU_SSIZE0						0x30    // s/t coord scale 
	#define SU_TSIZE0						0x31
	#define SU_SSIZE1						0x32
	#define SU_TSIZE1						0x33
	#define SU_SSIZE2						0x34
	#define SU_TSIZE2						0x35
	#define SU_SSIZE3						0x36
	#define SU_TSIZE3						0x37
	#define SU_SSIZE4						0x38
	#define SU_TSIZE4						0x39
	#define SU_SSIZE5						0x3A
	#define SU_TSIZE5						0x3B
	#define SU_SSIZE6						0x3C
	#define SU_TSIZE6						0x3D
	#define SU_SSIZE7						0x3E
	#define SU_TSIZE7						0x3F

	#define PE_ZMODE						0x40
	#define PE_CMODE0                       0x41
	#define PE_CMODE1                       0x42
	#define PE_CONTROL						0x43

	#define PE_DONE                         0x45
	#define PE_TOKEN                        0x47
	#define PE_TOKEN_INT                    0x48
	#define PE_COPY_CLEAR_AR                0x4F
	#define PE_COPY_CLEAR_GB                0x50
	#define PE_COPY_CLEAR_Z                 0x51
	#define PE_COPY_CMD						0x52

	#define TX_LOADTLUT0                    0x64    // tlut base in memory
	#define TX_LOADTLUT1                    0x65    // tmem ofs and size

	#define TX_SETMODE0_I0					0x80    // wrap (mode)
	#define TX_SETMODE0_I1					0x81
	#define TX_SETMODE0_I2					0x82
	#define TX_SETMODE0_I3					0x83

	#define TX_SETMODE1_I0					0x84
	#define TX_SETMODE1_I1					0x85
	#define TX_SETMODE1_I2					0x86
	#define TX_SETMODE1_I3					0x87

	#define TX_SETIMAGE0_I0                 0x88    // texture width, height, format
	#define TX_SETIMAGE0_I1                 0x89
	#define TX_SETIMAGE0_I2                 0x8A
	#define TX_SETIMAGE0_I3                 0x8B

	#define TX_SETIMAGE1_I0					0x8C
	#define TX_SETIMAGE1_I1					0x8D
	#define TX_SETIMAGE1_I2					0x8E
	#define TX_SETIMAGE1_I3					0x8F

	#define TX_SETIMAGE2_I0					0x90
	#define TX_SETIMAGE2_I1					0x91
	#define TX_SETIMAGE2_I2					0x92
	#define TX_SETIMAGE2_I3					0x93

	#define TX_SETIMAGE3_I0                 0x94    // texture_map >> 5, physical address
	#define TX_SETIMAGE3_I1                 0x95
	#define TX_SETIMAGE3_I2                 0x96
	#define TX_SETIMAGE3_I3                 0x97

	#define TX_SETTLUT_I0                   0x98    // bind tlut with texture
	#define TX_SETTLUT_I1                   0x99
	#define TX_SETTLUT_I2                   0x9A
	#define TX_SETTLUT_I3                   0x9B

	#define TX_SETMODE0_I4					0xA0
	#define TX_SETMODE0_I5					0xA1
	#define TX_SETMODE0_I6					0xA2
	#define TX_SETMODE0_I7					0xA3

	#define TX_SETMODE1_I4					0xA4
	#define TX_SETMODE1_I5					0xA5
	#define TX_SETMODE1_I6					0xA6
	#define TX_SETMODE1_I7					0xA7

	#define TX_SETIMAGE0_I4                 0xA8
	#define TX_SETIMAGE0_I5                 0xA9
	#define TX_SETIMAGE0_I6                 0xAA
	#define TX_SETIMAGE0_I7                 0xAB

	#define TX_SETIMAGE1_I4					0xAC
	#define TX_SETIMAGE1_I5					0xAD
	#define TX_SETIMAGE1_I6					0xAE
	#define TX_SETIMAGE1_I7					0xAF

	#define TX_SETIMAGE2_I4					0xB0
	#define TX_SETIMAGE2_I5					0xB1
	#define TX_SETIMAGE2_I6					0xB2
	#define TX_SETIMAGE2_I7					0xB3

	#define TX_SETIMAGE3_I4                 0xB4
	#define TX_SETIMAGE3_I5                 0xB5
	#define TX_SETIMAGE3_I6                 0xB6
	#define TX_SETIMAGE3_I7                 0xB7

	#define TX_SETTLUT_I4                   0xB8
	#define TX_SETTLUT_I5                   0xB9
	#define TX_SETTLUT_I6                   0xBA
	#define TX_SETTLUT_I7                   0xBB

	#define TEV_COLOR_ENV_0					0xC0
	#define TEV_ALPHA_ENV_0					0xC1
	#define TEV_COLOR_ENV_1					0xC2
	#define TEV_ALPHA_ENV_1					0xC3
	#define TEV_COLOR_ENV_2					0xC4
	#define TEV_ALPHA_ENV_2					0xC5
	#define TEV_COLOR_ENV_3					0xC6
	#define TEV_ALPHA_ENV_3					0xC7
	#define TEV_COLOR_ENV_4					0xC8
	#define TEV_ALPHA_ENV_4					0xC9
	#define TEV_COLOR_ENV_5					0xCA
	#define TEV_ALPHA_ENV_5					0xCB
	#define TEV_COLOR_ENV_6					0xCC
	#define TEV_ALPHA_ENV_6					0xCD
	#define TEV_COLOR_ENV_7					0xCE
	#define TEV_ALPHA_ENV_7					0xCF
	#define TEV_COLOR_ENV_8					0xD0
	#define TEV_ALPHA_ENV_8					0xD1
	#define TEV_COLOR_ENV_9					0xD2
	#define TEV_ALPHA_ENV_9					0xD3
	#define TEV_COLOR_ENV_A					0xD4
	#define TEV_ALPHA_ENV_A					0xD5
	#define TEV_COLOR_ENV_B					0xD6
	#define TEV_ALPHA_ENV_B					0xD7
	#define TEV_COLOR_ENV_C					0xD8
	#define TEV_ALPHA_ENV_C					0xD9
	#define TEV_COLOR_ENV_D					0xDA
	#define TEV_ALPHA_ENV_D					0xDB
	#define TEV_COLOR_ENV_E					0xDC
	#define TEV_ALPHA_ENV_E					0xDD
	#define TEV_COLOR_ENV_F					0xDE
	#define TEV_ALPHA_ENV_F					0xDF
	#define TEV_REGISTERL_0					0xE0
	#define TEV_REGISTERH_0					0xE1
	#define TEV_REGISTERL_1					0xE2
	#define TEV_REGISTERH_1					0xE3
	#define TEV_REGISTERL_2					0xE4
	#define TEV_REGISTERH_2					0xE5
	#define TEV_REGISTERL_3					0xE6
	#define TEV_REGISTERH_3					0xE7

	#define TEV_FOG_PARAM_0					0xEE
	#define TEV_FOG_PARAM_1					0xEF
	#define TEV_FOG_PARAM_2					0xF0
	#define TEV_FOG_PARAM_3					0xF1
	#define TEV_FOG_COLOR					0xF2

	#define TEV_ALPHAFUNC					0xF3
	#define TEV_Z_ENV_0						0xF4
	#define TEV_Z_ENV_1						0xF5
	#define TEV_KSEL_0						0xF6
	#define TEV_KSEL_1						0xF7
	#define TEV_KSEL_2						0xF8
	#define TEV_KSEL_3						0xF9
	#define TEV_KSEL_4						0xFA
	#define TEV_KSEL_5						0xFB
	#define TEV_KSEL_6						0xFC
	#define TEV_KSEL_7						0xFD

	// Vertex attributes

	enum class VertexAttr
	{
		VTX_POSMATIDX = 0,      // Position/Normal Matrix Index
		VTX_TEX0MTXIDX,         // Texture Coordinate 0 Matrix Index
		VTX_TEX1MTXIDX,         // Texture Coordinate 1 Matrix Index
		VTX_TEX2MTXIDX,         // Texture Coordinate 2 Matrix Index
		VTX_TEX3MTXIDX,         // Texture Coordinate 3 Matrix Index
		VTX_TEX4MTXIDX,         // Texture Coordinate 4 Matrix Index
		VTX_TEX5MTXIDX,         // Texture Coordinate 5 Matrix Index
		VTX_TEX6MTXIDX,         // Texture Coordinate 6 Matrix Index
		VTX_TEX7MTXIDX,         // Texture Coordinate 7 Matrix Index
		VTX_POS,                // Position
		VTX_NRM,                // Normal or Normal/Binormal/Tangent
		VTX_COLOR0,             // Color 0
		VTX_COLOR1,             // Color 1
		VTX_TEXCOORD0,          // Texture Coordinate 0
		VTX_TEXCOORD1,          // Texture Coordinate 1
		VTX_TEXCOORD2,          // Texture Coordinate 2
		VTX_TEXCOORD3,          // Texture Coordinate 3
		VTX_TEXCOORD4,          // Texture Coordinate 4
		VTX_TEXCOORD5,          // Texture Coordinate 5
		VTX_TEXCOORD6,          // Texture Coordinate 6
		VTX_TEXCOORD7,          // Texture Coordinate 7
		VTX_MAX_ATTR
	};

	// Attribute types (from VCD register)

	enum class AttrType
	{
		VCD_NONE = 0,           // attribute stage disabled
		VCD_DIRECT,             // direct data
		VCD_INDEX8,				// 8-bit indexed data
		VCD_INDEX16             // 16-bit indexed data (rare)
	};

	// Vertex Components Count (from VAT register)

	enum class VatCount
	{
		VCNT_POS_XY = 0,
		VCNT_POS_XYZ = 1,
		VCNT_NRM_XYZ = 0,
		VCNT_NRM_NBT = 1,    // index is NBT
		VCNT_NRM_NBT3 = 2,    // index is one from N/B/T
		VCNT_CLR_RGB = 0,
		VCNT_CLR_RGBA = 1,
		VCNT_TEX_S = 0,
		VCNT_TEX_ST = 1
	};

	// Vertex Component Format (from VAT register)

	enum class VatFormat
	{
		// For Components (coords)
		VFMT_U8 = 0,
		VFMT_S8 = 1,
		VFMT_U16 = 2,
		VFMT_S16 = 3,
		VFMT_F32 = 4,

		// For Colors
		VFMT_RGB565 = 0,
		VFMT_RGB8 = 1,
		VFMT_RGBX8 = 2,
		VFMT_RGBA4 = 3,
		VFMT_RGBA6 = 4,
		VFMT_RGBA8 = 5
	};

	// Texture generator types

	// Texgen Source

	// Lighting diffuse function

	// Lighting attenuation function
	
	// Lighting spotlight function

	// Lighting distance attenuation function

	// Texture offset

	// Texture Culling mode

	// Texture Clip mode

	// Texture Wrap mode

	// Texture filter

	// Texture format

	// Tlut format

	// Tlut size

	// Indirect texture format

	// Indirect texture bias select

	// Indirect texture alpha select

	// Indirect texture wrap

	// Indirect texture scale

	// TEV Reg ID

	// TEV Op

	// TEV Color argument

	// TEV Alpha argument

	// TEV Bias

	// TEV Clamp mode

	// TEV KColor select

	// TEV KAlpha select

	// TEV Color channel

	// Alpha Op

	// TEV Scale

	// Fog type

	// Blend mode

	// Blend factor

	// Compare

	// Logic Op

	// Pixel format

	// Z format

	// TEV Mode

	// Projection type

	// Tex Op

	// Alpha read mode

	// PE Registers (from CPU side). 16-bit access.

	enum class PEMappedRegister
	{
		PE_POKE_ZMODE_ID = 0,		// Cpu2Efb Z mode
		PE_POKE_CMODE0_ID,			// Cpu2Efb Color mode 0
		PE_POKE_CMODE1_ID,			// Cpu2Efb Color mode 1
		PE_POKE_AMODE0_ID,			// Cpu2Efb Alpha mode 0
		PE_POKE_AMODE1_ID,			// Cpu2Efb Alpha mode 1
		PE_SR_ID,					// Status register
		PE_UNK6_ID,
		PE_TOKEN_ID,				// Last token value
	};

	// PE status register
	#define PE_SR_DONE      (1 << 0)
	#define PE_SR_TOKEN     (1 << 1)
	#define PE_SR_DONEMSK   (1 << 2)
	#define PE_SR_TOKENMSK  (1 << 3)

}