#pragma once

#include <UnigineString.h>
#include <UnigineMeshDynamic.h>
#include <UnigineRender.h>
#include <NsRender/RenderDevice.h>

// Noesis vertex size tables
static constexpr uint8_t k_vertex_for_shader[Noesis::Shader::Count] =
{
	0, 0, 0, 1, 2, 2, 2, 3, 4, 4, 4, 4, 5, 6, 6, 6, 7, 8, 8, 8, 8, 9, 10, 10, 10, 11, 12, 12, 12,
	12, 9, 10, 10, 10, 11, 12, 12, 12, 12, 13, 14, 14, 14, 15, 16, 16, 16, 16, 17, 18, 19, 13, 20
};

static constexpr uint8_t k_format_for_vertex[Noesis::Shader::Vertex::Count] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 9, 10, 11, 12, 13, 10, 14, 15
};

static constexpr uint8_t k_size_for_format[Noesis::Shader::Vertex::Format::Count] =
{
	8, 12, 16, 24, 40, 16, 20, 28, 44, 20, 24, 32, 48, 28, 28, 44
};

inline uint8_t GetVertexFormatIndex(uint8_t shader_v)
{
	return k_format_for_vertex[k_vertex_for_shader[shader_v]];
}

inline uint8_t GetVertexSize(uint8_t shader_v)
{
	return k_size_for_format[GetVertexFormatIndex(shader_v)];
}

// Vertex input layout tables for MeshDynamic::setVertexFormat().
// Order of attributes MUST match Noesis vertex packing — see Noesis::Shader::Vertex::Format.
namespace NoesisVertexFormats
{
	using Attr = Unigine::MeshDynamic::Attribute;
	enum { F = Unigine::MeshDynamic::TYPE_FLOAT,
		U8 = Unigine::MeshDynamic::TYPE_UCHAR,
		U16 = Unigine::MeshDynamic::TYPE_USHORT };

	// 0: Pos
	static constexpr Attr kFormat0[]  = { {0, F, 2} };
	// 1: PosColor
	static constexpr Attr kFormat1[]  = { {0, F, 2}, {8, U8, 4} };
	// 2: PosTex0
	static constexpr Attr kFormat2[]  = { {0, F, 2}, {8, F, 2} };
	// 3: PosTex0Rect
	static constexpr Attr kFormat3[]  = { {0, F, 2}, {8, F, 2}, {16, U16, 4} };
	// 4: PosTex0RectTile
	static constexpr Attr kFormat4[]  = { {0, F, 2}, {8, F, 2}, {16, U16, 4}, {24, F, 4} };
	// 5: PosColorCoverage
	static constexpr Attr kFormat5[]  = { {0, F, 2}, {8, U8, 4}, {12, F, 1} };
	// 6: PosTex0Coverage
	static constexpr Attr kFormat6[]  = { {0, F, 2}, {8, F, 2}, {16, F, 1} };
	// 7: PosTex0CoverageRect
	static constexpr Attr kFormat7[]  = { {0, F, 2}, {8, F, 2}, {16, F, 1}, {20, U16, 4} };
	// 8: PosTex0CoverageRectTile
	static constexpr Attr kFormat8[]  = { {0, F, 2}, {8, F, 2}, {16, F, 1}, {20, U16, 4}, {28, F, 4} };
	// 9: PosColorTex1
	static constexpr Attr kFormat9[]  = { {0, F, 2}, {8, U8, 4}, {12, F, 2} };
	// 10: PosTex0Tex1
	static constexpr Attr kFormat10[] = { {0, F, 2}, {8, F, 2}, {16, F, 2} };
	// 11: PosTex0Tex1Rect
	static constexpr Attr kFormat11[] = { {0, F, 2}, {8, F, 2}, {16, F, 2}, {24, U16, 4} };
	// 12: PosTex0Tex1RectTile
	static constexpr Attr kFormat12[] = { {0, F, 2}, {8, F, 2}, {16, F, 2}, {24, U16, 4}, {32, F, 4} };
	// 13: PosColorTex0Tex1
	static constexpr Attr kFormat13[] = { {0, F, 2}, {8, U8, 4}, {12, F, 2}, {20, F, 2} };
	// 14: PosColorTex1Rect
	static constexpr Attr kFormat14[] = { {0, F, 2}, {8, U8, 4}, {12, F, 2}, {20, U16, 4} };
	// 15: PosColorTex0RectImagePos
	static constexpr Attr kFormat15[] = { {0, F, 2}, {8, U8, 4}, {12, F, 2}, {20, U16, 4}, {28, F, 4} };
}

struct VertexFormatDesc
{
	const Unigine::MeshDynamic::Attribute* attributes;
	int num_attributes;
};

#define NOESIS_FMT(N) { NoesisVertexFormats::kFormat##N, (int)(sizeof(NoesisVertexFormats::kFormat##N) / sizeof(Unigine::MeshDynamic::Attribute)) }
static constexpr VertexFormatDesc kVertexFormatTable[Noesis::Shader::Vertex::Format::Count] =
{
	NOESIS_FMT(0),  NOESIS_FMT(1),  NOESIS_FMT(2),  NOESIS_FMT(3),
	NOESIS_FMT(4),  NOESIS_FMT(5),  NOESIS_FMT(6),  NOESIS_FMT(7),
	NOESIS_FMT(8),  NOESIS_FMT(9),  NOESIS_FMT(10), NOESIS_FMT(11),
	NOESIS_FMT(12), NOESIS_FMT(13), NOESIS_FMT(14), NOESIS_FMT(15),
};
#undef NOESIS_FMT

inline Unigine::String GetShaderDefines(Noesis::Shader::Enum shader_type)
{
	using Shader = Noesis::Shader;
	Unigine::String defines;
	defines += "HLSL_WRAPPER,";
	if (Unigine::Render::getAPI() == Unigine::Render::API_VULKAN)
		defines += "VULKAN,";
	defines += Unigine::String::format("NOESIS_VERTEX_FORMAT=%d,",
		(int)GetVertexFormatIndex((uint8_t)shader_type));

	switch (shader_type)
	{
		case Shader::RGBA:          defines += "EFFECT_RGBA,";                                                       return defines;
		case Shader::Mask:          defines += "EFFECT_MASK,";                                                       return defines;
		case Shader::Clear:         defines += "EFFECT_CLEAR,";                                                      return defines;
		case Shader::Upsample:      defines += "EFFECT_UPSAMPLE,HAS_COLOR,HAS_UV0,HAS_UV1,";                         return defines;
		case Shader::Downsample:    defines += "EFFECT_DOWNSAMPLE,HAS_UV0,HAS_UV1,HAS_UV2,HAS_UV3,";                 return defines;
		case Shader::Shadow:        defines += "EFFECT_SHADOW,PAINT_SOLID,HAS_COLOR,HAS_UV1,HAS_RECT,";              return defines;
		case Shader::Blur:          defines += "EFFECT_BLUR,PAINT_SOLID,HAS_COLOR,HAS_UV1,";                         return defines;
		case Shader::Custom_Effect: defines += "EFFECT_CUSTOM,PAINT_SOLID,HAS_COLOR,";                               return defines;
		default: break;
	}

	bool has_color = false, has_uv0 = false, has_uv1 = false;
	bool has_st1 = false, has_coverage = false, has_rect = false, has_tile = false;

	// Paint kind + pattern modifier
	switch (shader_type)
	{
		case Shader::Path_Solid:
		case Shader::Path_AA_Solid:
		case Shader::SDF_Solid:
		case Shader::SDF_LCD_Solid:
		case Shader::Opacity_Solid:
			defines += "PAINT_SOLID,";
			has_color = true;
			break;

		case Shader::Path_Linear:
		case Shader::Path_AA_Linear:
		case Shader::SDF_Linear:
		case Shader::SDF_LCD_Linear:
		case Shader::Opacity_Linear:
			defines += "PAINT_LINEAR,";
			has_uv0 = true;
			break;

		case Shader::Path_Radial:
		case Shader::Path_AA_Radial:
		case Shader::SDF_Radial:
		case Shader::SDF_LCD_Radial:
		case Shader::Opacity_Radial:
			defines += "PAINT_RADIAL,";
			has_uv0 = true;
			break;

		case Shader::Path_Pattern:
		case Shader::Path_AA_Pattern:
		case Shader::SDF_Pattern:
		case Shader::SDF_LCD_Pattern:
		case Shader::Opacity_Pattern:
			defines += "PAINT_PATTERN,";
			has_uv0 = true;
			break;

		case Shader::Path_Pattern_Clamp:
		case Shader::Path_AA_Pattern_Clamp:
		case Shader::SDF_Pattern_Clamp:
		case Shader::SDF_LCD_Pattern_Clamp:
		case Shader::Opacity_Pattern_Clamp:
			defines += "PAINT_PATTERN,CLAMP_PATTERN,";
			has_uv0 = has_rect = true;
			break;

		case Shader::Path_Pattern_Repeat:
		case Shader::Path_AA_Pattern_Repeat:
		case Shader::SDF_Pattern_Repeat:
		case Shader::SDF_LCD_Pattern_Repeat:
		case Shader::Opacity_Pattern_Repeat:
			defines += "PAINT_PATTERN,REPEAT_PATTERN,";
			has_uv0 = has_rect = has_tile = true;
			break;

		case Shader::Path_Pattern_MirrorU:
		case Shader::Path_AA_Pattern_MirrorU:
		case Shader::SDF_Pattern_MirrorU:
		case Shader::SDF_LCD_Pattern_MirrorU:
		case Shader::Opacity_Pattern_MirrorU:
			defines += "PAINT_PATTERN,MIRRORU_PATTERN,";
			has_uv0 = has_rect = has_tile = true;
			break;

		case Shader::Path_Pattern_MirrorV:
		case Shader::Path_AA_Pattern_MirrorV:
		case Shader::SDF_Pattern_MirrorV:
		case Shader::SDF_LCD_Pattern_MirrorV:
		case Shader::Opacity_Pattern_MirrorV:
			defines += "PAINT_PATTERN,MIRRORV_PATTERN,";
			has_uv0 = has_rect = has_tile = true;
			break;

		case Shader::Path_Pattern_Mirror:
		case Shader::Path_AA_Pattern_Mirror:
		case Shader::SDF_Pattern_Mirror:
		case Shader::SDF_LCD_Pattern_Mirror:
		case Shader::Opacity_Pattern_Mirror:
			defines += "PAINT_PATTERN,MIRROR_PATTERN,";
			has_uv0 = has_rect = has_tile = true;
			break;

		default:
			// Unknown paint shader — fallback to PAINT_SOLID
			defines += "PAINT_SOLID,";
			has_color = true;
			break;
	}

	// Effect kind
	switch (shader_type)
	{
		case Shader::Path_Solid:
		case Shader::Path_Linear:
		case Shader::Path_Radial:
		case Shader::Path_Pattern:
		case Shader::Path_Pattern_Clamp:
		case Shader::Path_Pattern_Repeat:
		case Shader::Path_Pattern_MirrorU:
		case Shader::Path_Pattern_MirrorV:
		case Shader::Path_Pattern_Mirror:
			defines += "EFFECT_PATH,";
			break;

		case Shader::Path_AA_Solid:
		case Shader::Path_AA_Linear:
		case Shader::Path_AA_Radial:
		case Shader::Path_AA_Pattern:
		case Shader::Path_AA_Pattern_Clamp:
		case Shader::Path_AA_Pattern_Repeat:
		case Shader::Path_AA_Pattern_MirrorU:
		case Shader::Path_AA_Pattern_MirrorV:
		case Shader::Path_AA_Pattern_Mirror:
			defines += "EFFECT_PATH_AA,";
			has_coverage = true;
			break;

		case Shader::SDF_Solid:
		case Shader::SDF_Linear:
		case Shader::SDF_Radial:
		case Shader::SDF_Pattern:
		case Shader::SDF_Pattern_Clamp:
		case Shader::SDF_Pattern_Repeat:
		case Shader::SDF_Pattern_MirrorU:
		case Shader::SDF_Pattern_MirrorV:
		case Shader::SDF_Pattern_Mirror:
			defines += "EFFECT_SDF,";
			has_uv1 = has_st1 = true;
			break;

		case Shader::SDF_LCD_Solid:
		case Shader::SDF_LCD_Linear:
		case Shader::SDF_LCD_Radial:
		case Shader::SDF_LCD_Pattern:
		case Shader::SDF_LCD_Pattern_Clamp:
		case Shader::SDF_LCD_Pattern_Repeat:
		case Shader::SDF_LCD_Pattern_MirrorU:
		case Shader::SDF_LCD_Pattern_MirrorV:
		case Shader::SDF_LCD_Pattern_Mirror:
			defines += "EFFECT_SDF_LCD,";
			has_uv1 = has_st1 = true;
			break;

		case Shader::Opacity_Solid:
		case Shader::Opacity_Linear:
		case Shader::Opacity_Radial:
		case Shader::Opacity_Pattern:
		case Shader::Opacity_Pattern_Clamp:
		case Shader::Opacity_Pattern_Repeat:
		case Shader::Opacity_Pattern_MirrorU:
		case Shader::Opacity_Pattern_MirrorV:
		case Shader::Opacity_Pattern_Mirror:
			defines += "EFFECT_OPACITY,";
			has_uv1 = true;
			break;

		default:
			// Non-paint shaders already returned above, so this should not happen
			// for normal operation. Fallback to EFFECT_PATH for safety.
			defines += "EFFECT_PATH,";
			break;
	}

	if (has_color)    defines += "HAS_COLOR,";
	if (has_uv0)      defines += "HAS_UV0,";
	if (has_uv1)      defines += "HAS_UV1,";
	if (has_st1)      defines += "HAS_ST1,";
	if (has_coverage) defines += "HAS_COVERAGE,";
	if (has_rect)     defines += "HAS_RECT,";
	if (has_tile)     defines += "HAS_TILE,";
	return defines;
}
