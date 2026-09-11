#include "NoesisRenderDevice.h"
#include "NoesisShader.h"

#include <UnigineRender.h>
#include <UnigineImage.h>
#include <UnigineLog.h>

using namespace Unigine;

//////////////////////////////////////////////////////////////////////////
// NoesisRenderDevice
//////////////////////////////////////////////////////////////////////////

NoesisRenderDevice::NoesisRenderDevice(int color_format, int depth_stencil_format)
	: color_format_(color_format)
	, depth_stencil_format_(depth_stencil_format)
{
	// mesh_dynamic_per_format_ allocated lazily in DrawBatch per Noesis vertex format.
}

NoesisRenderDevice::~NoesisRenderDevice() = default;

const Noesis::DeviceCaps& NoesisRenderDevice::GetCaps() const
{
	static Noesis::DeviceCaps caps;
	caps.linearRendering = false;
	return caps;
}

Noesis::Ptr<Noesis::Texture> NoesisRenderDevice::CreateTexture(const char* label,
	uint32_t width, uint32_t height, uint32_t numLevels,
	Noesis::TextureFormat::Enum noesis_format, const void** data)
{
	int texture_format = Texture::FORMAT_RGBA8;
	bool use_alpha = true;
	switch (noesis_format)
	{
	case Noesis::TextureFormat::R8:    texture_format = Texture::FORMAT_R8;    break;
	case Noesis::TextureFormat::RGBA8: texture_format = Texture::FORMAT_RGBA8; break;
	case Noesis::TextureFormat::RGBX8:
		texture_format = Texture::FORMAT_RGBA8;
		use_alpha = false;
		break;
	default: texture_format = Texture::FORMAT_R8; break;
	}

	TexturePtr tex = Texture::create();
	tex->create2D((int)width, (int)height, texture_format);
	tex->setDebugName(label);

	Noesis::Ptr<NoesisTexture> noesis_tex = Noesis::MakePtr<NoesisTexture>(tex, use_alpha);

	if (data)
	{
		uint32_t w = width, h = height;
		for (uint32_t level = 0; level < numLevels; level++)
		{
			UpdateTexture(noesis_tex, level, 0, 0, w, h, data[level]);
			w >>= 1;
			h >>= 1;
		}
	}

	return noesis_tex;
}

Noesis::Ptr<Noesis::RenderTarget> NoesisRenderDevice::CreateRenderTarget(
	const char* label, uint32_t width, uint32_t height, uint32_t sampleCount, bool needsStencil)
{
	UNIGINE_UNUSED(sampleCount);

	TexturePtr color = Texture::create();
	color->create2D((int)width, (int)height, color_format_, Texture::SAMPLER_FILTER_LINEAR | Texture::FORMAT_USAGE_RENDER);
	color->setDebugName(String::format("%s: Noesis Color AA", label).get());

	TexturePtr depth_stencil;
	if (needsStencil)
	{
		depth_stencil = Texture::create();
		depth_stencil->create2D((int)width, (int)height, depth_stencil_format_, Texture::FORMAT_USAGE_RENDER);
		depth_stencil->setDebugName(String::format("%s: Noesis Depth Stencil", label).get());
	}

	return create_render_target(label, width, height, color, depth_stencil);
}

void* NoesisRenderDevice::MapVertices(uint32_t bytes)
{
	vertex_staging_buffer_.resize(bytes);
	return vertex_staging_buffer_.empty() ? nullptr : &vertex_staging_buffer_[0];
}

void NoesisRenderDevice::UnmapVertices()
{
	// GPU upload happens in DrawBatch via mesh_dynamic_->setVertexArray + flushVertex
}

void* NoesisRenderDevice::MapIndices(uint32_t bytes)
{
	index_staging_buffer_.resize(bytes);
	return index_staging_buffer_.empty() ? nullptr : &index_staging_buffer_[0];
}

void NoesisRenderDevice::UnmapIndices()
{
	// GPU upload happens in DrawBatch via mesh_dynamic_->setIndicesArray + flushIndices
}

void NoesisRenderDevice::SetRenderTarget(Noesis::RenderTarget* surface)
{
	current_render_target_ = (NoesisRenderTarget*)surface;
	if (current_render_target_)
		current_render_target_->bindTextures();
}

void NoesisRenderDevice::SetShaderTexture(int slot, Noesis::Texture *texture, Noesis::SamplerState sampler, const char *name)
{
	if (!texture)
		return;

	int sampler_flags = 0;

	if (sampler.f.wrapMode == Noesis::WrapMode::ClampToEdge)
		sampler_flags |= Unigine::Texture::SAMPLER_WRAP_CLAMP;
	else if (sampler.f.wrapMode == Noesis::WrapMode::ClampToZero)
		sampler_flags |= Unigine::Texture::SAMPLER_WRAP_BORDER;

	if (sampler.f.minmagFilter == Noesis::MinMagFilter::Nearest)
	{
		int filter_flags[3] = {Unigine::Texture::SAMPLER_FILTER_POINT, Unigine::Texture::SAMPLER_FILTER_POINT, Unigine::Texture::SAMPLER_FILTER_LINEAR};
		sampler_flags |= filter_flags[sampler.f.mipFilter];
	}
	else if (sampler.f.minmagFilter == Noesis::MinMagFilter::Linear)
	{
		int filter_flags[3] = {Unigine::Texture::SAMPLER_FILTER_BILINEAR, Unigine::Texture::SAMPLER_FILTER_BILINEAR, Unigine::Texture::SAMPLER_FILTER_TRILINEAR};
		sampler_flags |= filter_flags[sampler.f.mipFilter];
	}

	TexturePtr shader_texture = static_cast<NoesisTexture *>(texture)->getTexturePtr();
	shader_texture->setSamplerFlags(sampler_flags);
	shader_texture->setDebugName(name);

	RenderState::setTexture(RenderState::BIND_FRAGMENT, slot, shader_texture);
}

void NoesisRenderDevice::BeginOffscreenRender()
{
	RenderState::saveState();
	RenderState::clearStates();
}

void NoesisRenderDevice::EndOffscreenRender()
{
	RenderState::restoreState();
}

void NoesisRenderDevice::BeginOnscreenRender()
{
	RenderState::saveState();
	RenderState::clearStates();
}

void NoesisRenderDevice::EndOnscreenRender()
{
	RenderState::restoreState();
}

ShaderPtr NoesisRenderDevice::get_or_compile_shader(Noesis::Shader::Enum shader_type)
{
	auto it = shader_cache_.find((uint8_t)shader_type);
	if (it != shader_cache_.end())
		return it->data;

	static constexpr const char *VS_PATH = "noesis/shaders/noesis_gui.vert";
	static constexpr const char *PS_PATH = "noesis/shaders/noesis_gui.frag";

	String defines = GetShaderDefines(shader_type);

	ShaderPtr shader = Shader::create();
	if (!shader->compileVertFrag(VS_PATH, PS_PATH, defines.get()))
	{
		Log::error("[Noesis] Shader compile failed for type %u\nDefines: %s\n", shader_type, defines.get());
		return nullptr;
	}

	shader_cache_[(uint8_t)shader_type] = shader;
	return shader;
}

void NoesisRenderDevice::apply_render_state(Noesis::RenderState state, uint8_t stencil_ref)
{
	// Blend
	if (!state.f.colorEnable)
	{
		// Stencil-only pass: preserve color
		RenderState::setBlendFunc(RenderState::BLEND_ZERO, RenderState::BLEND_ONE);
	}
	else
	{
		switch (state.f.blendMode)
		{
		case Noesis::BlendMode::Src:
			RenderState::setBlendFunc(RenderState::BLEND_ONE, RenderState::BLEND_ZERO);
			break;
		case Noesis::BlendMode::SrcOver:
			RenderState::setBlendFunc(RenderState::BLEND_ONE, RenderState::BLEND_ONE_MINUS_SRC_ALPHA);
			break;
		case Noesis::BlendMode::SrcOver_Multiply:
			RenderState::setBlendFunc(RenderState::BLEND_DEST_COLOR, RenderState::BLEND_ONE_MINUS_SRC_ALPHA);
			break;
		case Noesis::BlendMode::SrcOver_Screen:
			RenderState::setBlendFunc(RenderState::BLEND_ONE, RenderState::BLEND_ONE_MINUS_SRC_COLOR);
			break;
		case Noesis::BlendMode::SrcOver_Additive:
			RenderState::setBlendFunc(RenderState::BLEND_ONE, RenderState::BLEND_ONE);
			break;
		case Noesis::BlendMode::SrcOver_Dual:
			RenderState::setBlendFunc(RenderState::BLEND_ONE, RenderState::BLEND_ONE_MINUS_SRC1_COLOR);
			break;
		default:
			RenderState::setBlendFunc(RenderState::BLEND_ONE, RenderState::BLEND_ONE_MINUS_SRC_ALPHA);
			break;
		}
	}

	// Wireframe
	RenderState::setPolygonFill(state.f.wireframe
		? RenderState::FILL_WIREFRAME
		: RenderState::FILL_SOLID);

	// Depth write always off for Noesis
	RenderState::setDepthWrite(false);

	// Stencil
	switch (state.f.stencilMode)
	{
	case Noesis::StencilMode::Disabled:
		RenderState::setStencilFunc(RenderState::STENCIL_NONE);
		break;
	case Noesis::StencilMode::Equal_Keep:
		RenderState::setStencilFunc(RenderState::STENCIL_EQUAL);
		RenderState::setStencilPass(RenderState::STENCIL_KEEP);
		RenderState::setStencilRef(stencil_ref);
		break;
	case Noesis::StencilMode::Equal_Incr:
		RenderState::setStencilFunc(RenderState::STENCIL_EQUAL);
		RenderState::setStencilPass(RenderState::STENCIL_INCR);
		RenderState::setStencilRef(stencil_ref);
		break;
	case Noesis::StencilMode::Equal_Decr:
		RenderState::setStencilFunc(RenderState::STENCIL_EQUAL);
		RenderState::setStencilPass(RenderState::STENCIL_DECR);
		RenderState::setStencilRef(stencil_ref);
		break;
	case Noesis::StencilMode::Clear:
		// Equivalent to D3D12_STENCIL_OP_ZERO: write 0 to stencil
		RenderState::setStencilFunc(RenderState::STENCIL_ALWAYS);
		RenderState::setStencilPass(RenderState::STENCIL_REPLACE);
		RenderState::setStencilRef(0);
		break;
	case Noesis::StencilMode::Disabled_ZTest:
		RenderState::setStencilFunc(RenderState::STENCIL_NONE);
		RenderState::setDepthFunc(RenderState::DEPTH_GEQUAL);
		break;
	case Noesis::StencilMode::Equal_Keep_ZTest:
		RenderState::setStencilFunc(RenderState::STENCIL_EQUAL);
		RenderState::setStencilPass(RenderState::STENCIL_KEEP);
		RenderState::setStencilRef(stencil_ref);
		RenderState::setDepthFunc(RenderState::DEPTH_GEQUAL);
		break;
	default:
		RenderState::setStencilFunc(RenderState::STENCIL_NONE);
		break;
	}
}

void NoesisRenderDevice::set_vertex_uniforms(ShaderPtr& shader, const Noesis::Batch& batch)
{
	// VS cbuffer b0: projectionMtx (16 floats, column-major float4x4)
	if (batch.vertexUniforms[0].values)
	{
		const float* proj = (const float*)batch.vertexUniforms[0].values;
		Math::mat4 proj_mtx;
		memcpy((void*)&proj_mtx, proj, sizeof(Math::mat4));
		shader->setParameterFloat4x4("projectionMtx", proj_mtx);
	}

	// VS cbuffer b1: textureSize (2 floats, only for SDF variants)
	if (batch.vertexUniforms[1].values)
	{
		const float* ts = (const float*)batch.vertexUniforms[1].values;
		shader->setParameterFloat2("textureSize", Math::vec2(ts[0], ts[1]));
	}
}

void NoesisRenderDevice::set_pixel_uniforms(ShaderPtr& shader,
	Noesis::Shader::Enum shader_type, const Noesis::Batch& batch)
{
	const float* ps0 = (const float*)batch.pixelUniforms[0].values;
	const float* ps1 = (const float*)batch.pixelUniforms[1].values;

	// EFFECT_RGBA: float4 rgba
	if (shader_type == Noesis::Shader::RGBA && ps0)
	{
		shader->setParameterFloat4("rgba", Math::vec4(ps0[0], ps0[1], ps0[2], ps0[3]));
		return;
	}

	// PAINT_RADIAL: float4 radialGrad0, float3 radialGrad1
	switch (shader_type)
	{
	case Noesis::Shader::Path_Radial:
	case Noesis::Shader::Path_AA_Radial:
	case Noesis::Shader::SDF_Radial:
	case Noesis::Shader::SDF_LCD_Radial:
	case Noesis::Shader::Opacity_Radial:
		if (ps0)
		{
			shader->setParameterFloat4("radialGrad0", Math::vec4(ps0[0], ps0[1], ps0[2], ps0[3]));
			shader->setParameterFloat3("radialGrad1", Math::vec3(ps0[4], ps0[5], ps0[6]));
		}
		return;
	default:
		break;
	}

	// PAINT_LINEAR: float opacity
	switch (shader_type)
	{
	case Noesis::Shader::Path_Linear:
	case Noesis::Shader::Path_AA_Linear:
	case Noesis::Shader::SDF_Linear:
	case Noesis::Shader::SDF_LCD_Linear:
	case Noesis::Shader::Opacity_Linear:
		if (ps0)
			shader->setParameterFloat("opacity", ps0[0]);
		return;
	default:
		break;
	}

	// PAINT_PATTERN: float opacity
	switch (shader_type)
	{
	case Noesis::Shader::Path_Pattern:
	case Noesis::Shader::Path_Pattern_Clamp:
	case Noesis::Shader::Path_Pattern_Repeat:
	case Noesis::Shader::Path_Pattern_MirrorU:
	case Noesis::Shader::Path_Pattern_MirrorV:
	case Noesis::Shader::Path_Pattern_Mirror:
	case Noesis::Shader::Path_AA_Pattern:
	case Noesis::Shader::Path_AA_Pattern_Clamp:
	case Noesis::Shader::Path_AA_Pattern_Repeat:
	case Noesis::Shader::Path_AA_Pattern_MirrorU:
	case Noesis::Shader::Path_AA_Pattern_MirrorV:
	case Noesis::Shader::Path_AA_Pattern_Mirror:
	case Noesis::Shader::SDF_Pattern:
	case Noesis::Shader::SDF_Pattern_Clamp:
	case Noesis::Shader::SDF_Pattern_Repeat:
	case Noesis::Shader::SDF_Pattern_MirrorU:
	case Noesis::Shader::SDF_Pattern_MirrorV:
	case Noesis::Shader::SDF_Pattern_Mirror:
	case Noesis::Shader::SDF_LCD_Pattern:
	case Noesis::Shader::SDF_LCD_Pattern_Clamp:
	case Noesis::Shader::SDF_LCD_Pattern_Repeat:
	case Noesis::Shader::SDF_LCD_Pattern_MirrorU:
	case Noesis::Shader::SDF_LCD_Pattern_MirrorV:
	case Noesis::Shader::SDF_LCD_Pattern_Mirror:
	case Noesis::Shader::Opacity_Pattern:
	case Noesis::Shader::Opacity_Pattern_Clamp:
	case Noesis::Shader::Opacity_Pattern_Repeat:
	case Noesis::Shader::Opacity_Pattern_MirrorU:
	case Noesis::Shader::Opacity_Pattern_MirrorV:
	case Noesis::Shader::Opacity_Pattern_Mirror:
		if (ps0)
			shader->setParameterFloat("opacity", ps0[0]);
		return;
	default:
		break;
	}

	// EFFECT_SHADOW: Buffer2 = float4 shadowColor, float2 shadowOffset, float blend
	if (shader_type == Noesis::Shader::Shadow && ps1)
	{
		shader->setParameterFloat4("shadowColor", Math::vec4(ps1[0], ps1[1], ps1[2], ps1[3]));
		shader->setParameterFloat2("shadowOffset", Math::vec2(ps1[4], ps1[5]));
		shader->setParameterFloat("blend", ps1[6]);
		return;
	}

	// EFFECT_BLUR: Buffer2 = float blend
	if (shader_type == Noesis::Shader::Blur && ps1)
	{
		shader->setParameterFloat("blend", ps1[0]);
		return;
	}

	// PAINT_SOLID and everything else: no PS uniforms needed
}

void NoesisRenderDevice::DrawBatch(const Noesis::Batch& batch)
{
	if (batch.numIndices == 0 || vertex_staging_buffer_.empty()
		|| index_staging_buffer_.empty())
		return;

	auto shader_type = (Noesis::Shader::Enum)batch.shader.v;
	ShaderPtr shader = get_or_compile_shader(shader_type);
	if (!shader)
		return;

	// Pick the MeshDynamic dedicated to this batch's vertex format. Sharing one
	// MeshDynamic across formats breaks on Vulkan.
	uint8_t vertex_format_idx = GetVertexFormatIndex(batch.shader.v);
	MeshDynamicPtr &mesh_dynamic = mesh_dynamic_per_format_[vertex_format_idx];
	if (!mesh_dynamic)
	{
		mesh_dynamic = MeshDynamic::create(MeshDynamic::USAGE_DYNAMIC_ALL);
		const auto& desc = kVertexFormatTable[vertex_format_idx];
		mesh_dynamic->setVertexFormat(desc.attributes, desc.num_attributes);
	}
	current_vertex_format_ = vertex_format_idx;

	// Upload only the slice of vertices this batch needs.
	const uint8_t* vertex_src = vertex_staging_buffer_.get() + batch.vertexOffset;
	mesh_dynamic->setVertexArray(vertex_src, (int)batch.numVertices);

	// Convert uint16 -> int for the slice this batch consumes.
	const uint16_t* index_src = (const uint16_t*)index_staging_buffer_.get();
	index_int_buffer_.resize((int)batch.numIndices);
	for (uint32_t i = 0; i < batch.numIndices; i++)
		index_int_buffer_[i] = (int)index_src[batch.startIndex + i];
	mesh_dynamic->setIndicesArray(index_int_buffer_.get(), (int)batch.numIndices);

	// Set shader textures
	SetShaderTexture(0, batch.pattern, batch.patternSampler, "Pattern");
	SetShaderTexture(1, batch.ramps,   batch.rampsSampler,	 "Ramps");
	SetShaderTexture(2, batch.image,   batch.imageSampler,	 "Image");
	SetShaderTexture(3, batch.glyphs,  batch.glyphsSampler,	 "Glyphs");
	SetShaderTexture(4, batch.shadow,  batch.shadowSampler,	 "Shadow");

	// Set uniforms
	set_vertex_uniforms(shader, batch);
	set_pixel_uniforms(shader, shader_type, batch);

	// Apply render state
	apply_render_state(batch.renderState, batch.stencilRef);

	RenderState::setShader(shader);
	RenderState::flushStates();

	mesh_dynamic->bind();
	mesh_dynamic->flushVertex();
	mesh_dynamic->flushIndices();
	mesh_dynamic->renderSurface(MeshDynamic::MODE_TRIANGLES, 0, 0, (int)batch.numIndices);
	mesh_dynamic->unbind();

	RenderState::clearTextures();
}

Noesis::Ptr<Noesis::RenderTarget> NoesisRenderDevice::CloneRenderTarget(
	const char* label, Noesis::RenderTarget* surface)
{
	auto* rt = (NoesisRenderTarget*)surface;
	TexturePtr color = rt->getColorTexture();
	TexturePtr depth = rt->getDepthTexture();
	return create_render_target(label, color->getWidth(), color->getHeight(), color, depth);
}

void NoesisRenderDevice::UpdateTexture(Noesis::Texture* texture_, uint32_t level,
	uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data)
{
	// Noesis dynamic textures (glyph atlas) are always single-mip in our backend:
	// CreateTexture uses Texture::create2D without FORMAT_MIPMAPS. If SDK ever
	// requests level > 0, our 1-mip Image would mismatch texture mip count and
	// setImage2D would fail loudly. Assert here for earlier diagnostics.
	UNIGINE_ASSERT(level == 0 && "NoesisRenderDevice::UpdateTexture: multi-mip textures not supported");

	auto* tex = (NoesisTexture*)texture_;
	TexturePtr texture_ptr = tex->getTexturePtr();
	int image_format = texture_ptr->getImageFormat();

	ImagePtr image = Image::create();
	image->create2D((int)width, (int)height, image_format, 1);
	memcpy(image->getPixels2D(0), data, image->getPixelsSize());
	texture_ptr->setImage2D(image, (int)x, (int)y);
}

void NoesisRenderDevice::BeginTile(Noesis::RenderTarget* surface, const Noesis::Tile& tile)
{
	UNIGINE_UNUSED(surface);
	UNIGINE_UNUSED(tile);
}

void NoesisRenderDevice::EndTile(Noesis::RenderTarget* surface)
{
	UNIGINE_UNUSED(surface);
}

void NoesisRenderDevice::ResolveRenderTarget(Noesis::RenderTarget* surface,
	const Noesis::Tile* tiles, uint32_t numTiles)
{
	auto* rt = (NoesisRenderTarget*)surface;
	UNIGINE_ASSERT(current_render_target_ == rt && "invalid render target");

	TexturePtr src = rt->getColorTexture();
	TexturePtr dst = rt->getShaderResourceTexture();

	for (uint32_t i = 0; i < numTiles; i++)
	{
		const Noesis::Tile& tile = tiles[i];
		Math::ivec3 offset;
		offset.x = (int)tile.x;
		offset.y = dst->getHeight() - (int)(tile.y + tile.height);
		offset.z = 0;
		dst->copyRegion(src, offset, 0, offset, 0, (int)tile.width, (int)tile.height, 1);
	}

	if (current_render_target_)
	{
		current_render_target_->unbindTextures();
		current_render_target_ = nullptr;
	}
}

Noesis::Ptr<Noesis::RenderTarget> NoesisRenderDevice::create_render_target(
	const char* label, uint32_t width, uint32_t height,
	TexturePtr color_aa, TexturePtr depth_stencil)
{
	UNIGINE_UNUSED(label);
	TexturePtr srv = Texture::create();
	srv->create2D((int)width, (int)height, color_format_, Texture::SAMPLER_FILTER_LINEAR);
	srv->setDebugName(String::format("%s: Noesis SRV", label).get());
	return Noesis::MakePtr<NoesisRenderTarget>(srv, color_aa, depth_stencil);
}
