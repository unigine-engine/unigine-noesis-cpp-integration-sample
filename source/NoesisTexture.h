#pragma once

#include <NsRender/Texture.h>
#include <NsRender/RenderTarget.h>
#include <UnigineTextures.h>
#include <UnigineRender.h>

class NoesisTexture final: public Noesis::Texture
{
public:
	NoesisTexture(Unigine::TexturePtr texture_, bool use_alpha_ = true);

	Unigine::TexturePtr getTexturePtr() { return ptr; }

	uint32_t GetWidth() const override { return width; }
	uint32_t GetHeight() const override { return height; }
	bool HasMipMaps() const override { return num_mipmaps > 1; }
	bool IsInverted() const override { return false; }
	bool HasAlpha() const override { return has_alpha; }

private:
	Unigine::TexturePtr ptr;
	uint32_t width;
	uint32_t height;
	uint32_t num_mipmaps;
	bool has_alpha;
	bool use_alpha;
};

class NoesisRenderTarget final: public Noesis::RenderTarget
{
public:
	NoesisRenderTarget(Unigine::TexturePtr texture_,
		Unigine::TexturePtr color_out_, Unigine::TexturePtr depth_stencil_out_);

	Unigine::TexturePtr getColorTexture() { return color_out; }
	Unigine::TexturePtr getDepthTexture() { return depth_stencil_out; }
	Unigine::TexturePtr getShaderResourceTexture() { return texture->getTexturePtr(); }

	void bindTextures();
	void unbindTextures();

	Noesis::Texture* GetTexture() override { return texture; }

private:
	Noesis::Ptr<NoesisTexture> texture;
	Unigine::TexturePtr color_out;
	Unigine::TexturePtr depth_stencil_out;
	Unigine::RenderTargetPtr render_target;
};
