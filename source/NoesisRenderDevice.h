#pragma once

#include "NoesisTexture.h"

#include <NsRender/RenderDevice.h>
#include <UnigineVector.h>
#include <UnigineShader.h>
#include <UnigineHashMap.h>
#include <UnigineMeshDynamic.h>

class NoesisRenderDevice final: public Noesis::RenderDevice
{
public:
	NoesisRenderDevice(int color_format, int depth_stencil_format);
	~NoesisRenderDevice();

	const Noesis::DeviceCaps& GetCaps() const override;

	Noesis::Ptr<Noesis::Texture> CreateTexture(const char* label, uint32_t width,
		uint32_t height, uint32_t numLevels, Noesis::TextureFormat::Enum format,
		const void** data) override;

	Noesis::Ptr<Noesis::RenderTarget> CreateRenderTarget(const char* label,
		uint32_t width, uint32_t height, uint32_t sampleCount,
		bool needsStencil) override;

	void SetRenderTarget(Noesis::RenderTarget* surface) override;
	void SetShaderTexture(int slot, Noesis::Texture *texture, Noesis::SamplerState sampler, const char *name);

	void BeginOffscreenRender() override;
	void EndOffscreenRender() override;

	void BeginOnscreenRender() override;
	void EndOnscreenRender() override;

	void* MapVertices(uint32_t bytes) override;
	void UnmapVertices() override;

	void* MapIndices(uint32_t bytes) override;
	void UnmapIndices() override;

	void DrawBatch(const Noesis::Batch& batch) override;

	Noesis::Ptr<Noesis::RenderTarget> CloneRenderTarget(const char* label,
		Noesis::RenderTarget* surface) override;

	void UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x,
		uint32_t y, uint32_t width, uint32_t height, const void* data) override;

	void BeginTile(Noesis::RenderTarget* surface, const Noesis::Tile& tile) override;
	void EndTile(Noesis::RenderTarget* surface) override;

	void ResolveRenderTarget(Noesis::RenderTarget* surface, const Noesis::Tile* tiles,
		uint32_t numTiles) override;

private:
	Noesis::Ptr<Noesis::RenderTarget> create_render_target(const char* label,
		uint32_t width, uint32_t height, Unigine::TexturePtr color_aa,
		Unigine::TexturePtr depth_stencil);

	Unigine::ShaderPtr get_or_compile_shader(Noesis::Shader::Enum shader_type);

	void apply_render_state(Noesis::RenderState state, uint8_t stencil_ref);
	void set_vertex_uniforms(Unigine::ShaderPtr& shader, const Noesis::Batch& batch);
	void set_pixel_uniforms(Unigine::ShaderPtr& shader, Noesis::Shader::Enum shader_type,
		const Noesis::Batch& batch);

	Unigine::Vector<uint8_t> vertex_staging_buffer_;
	Unigine::Vector<uint8_t> index_staging_buffer_;
	Unigine::Vector<int>     index_int_buffer_; // uint16 -> int conversion scratch

	Unigine::MeshDynamicPtr mesh_dynamic_per_format_[Noesis::Shader::Vertex::Format::Count];
	uint8_t current_vertex_format_ = 0xFF; // forces setVertexFormat on first batch

	NoesisRenderTarget* current_render_target_ = nullptr;

	Unigine::HashMap<uint8_t, Unigine::ShaderPtr> shader_cache_;

	int color_format_{Unigine::Texture::FORMAT_RGBA8};
	int depth_stencil_format_{Unigine::Texture::FORMAT_D24S8};
};
