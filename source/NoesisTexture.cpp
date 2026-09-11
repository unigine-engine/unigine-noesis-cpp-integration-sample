#include "NoesisTexture.h"

NoesisTexture::NoesisTexture(Unigine::TexturePtr texture_, bool use_alpha_)
	: ptr(texture_)
	, width((uint32_t)texture_->getWidth(0))
	, height((uint32_t)texture_->getHeight(0))
	, num_mipmaps((uint32_t)texture_->getNumMipmaps())
	, has_alpha(texture_->getFormat() == Unigine::Texture::FORMAT_RGBA8)
	, use_alpha(use_alpha_)
{}

NoesisRenderTarget::NoesisRenderTarget(Unigine::TexturePtr texture_,
	Unigine::TexturePtr color_out_, Unigine::TexturePtr depth_stencil_out_)
	: color_out(color_out_)
	, depth_stencil_out(depth_stencil_out_)
{
	texture = *new NoesisTexture(texture_);
	render_target = Unigine::RenderTarget::create();
}

void NoesisRenderTarget::bindTextures()
{
	Unigine::RenderState::setTexture(Unigine::RenderState::BIND_ALL, 0, texture->getTexturePtr());
	render_target->bindColorTexture(0, color_out);
	if (depth_stencil_out)
		render_target->bindDepthTexture(depth_stencil_out);
	render_target->enable();
}

void NoesisRenderTarget::unbindTextures()
{
	render_target->disable();
	render_target->unbindAll();
	Unigine::RenderState::setTexture(Unigine::RenderState::BIND_ALL, 0, nullptr);
}
