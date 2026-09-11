#pragma once

#include "NoesisTexture.h"

#include <NsGui/CachedFontProvider.h>
#include <NsGui/XamlProvider.h>
#include <NsGui/TextureProvider.h>
#include <NsGui/MemoryStream.h>
#include <NsGui/Uri.h>
#include <NsCore/Ptr.h>

#include <UnigineString.h>
#include <UnigineVector.h>
#include <UnigineHashMap.h>

class NoesisFontProvider: public Noesis::CachedFontProvider
{
public:
	explicit NoesisFontProvider(const char* root_path);

	void register_face(const char* file_path, const char* family,
		Noesis::FontWeight weight, Noesis::FontStretch stretch, Noesis::FontStyle style);

protected:
	void ScanFolder(const Noesis::Uri& folder) override;
	Noesis::Ptr<Noesis::Stream> OpenFont(const Noesis::Uri& folder,
		const char* filename) const override;

private:
	Unigine::String root_path_;
	mutable Unigine::HashMap<Unigine::String, Unigine::Vector<uint8_t>> buffers_;
};

class NoesisXamlProvider final: public Noesis::XamlProvider
{
public:
	explicit NoesisXamlProvider(const char* root_path);

	Noesis::Ptr<Noesis::Stream> LoadXaml(const Noesis::Uri& uri) override;

private:
	Unigine::String root_path_;
	Unigine::HashMap<Unigine::String, Unigine::Vector<uint8_t>> buffers_;
};

class NoesisTextureProvider final: public Noesis::TextureProvider
{
public:
	explicit NoesisTextureProvider(const char* root_path);

	Noesis::TextureInfo GetTextureInfo(const Noesis::Uri& uri) override;
	Noesis::Ptr<Noesis::Texture> LoadTexture(const Noesis::Uri& uri,
		Noesis::RenderDevice* device) override;

private:
	bool load_image_rgba8(const Noesis::Uri& uri, Unigine::ImagePtr& out_image);

	Unigine::String root_path_;
};
