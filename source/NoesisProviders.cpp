#include "NoesisProviders.h"

#include <NsGui/MemoryStream.h>
#include <NsGui/Stream.h>
#include <NsGui/FontProperties.h>

#include <UnigineImage.h>
#include <UnigineLog.h>
#include <UnigineStreams.h>
#include <UnigineTextures.h>

using namespace Unigine;

namespace
{

bool read_file_bytes(const String& path, Vector<uint8_t>& out)
{
	FilePtr file = File::create();
	if (!file->open(path.get(), "rb"))
		return false;
	size_t size = file->getSize();
	out.resize((int)size);
	file->read(out.get(), size);
	file->close();
	return true;
}

} // namespace

//////////////////////////////////////////////////////////////////////////
// NoesisFontProvider
//////////////////////////////////////////////////////////////////////////

NoesisFontProvider::NoesisFontProvider(const char* root_path)
	: root_path_(root_path)
{}

void NoesisFontProvider::register_face(const char* file_path, const char* family,
	Noesis::FontWeight weight, Noesis::FontStretch stretch, Noesis::FontStyle style)
{
	const char *separator = ::strrchr(file_path, '/');
	const String folder = separator ? String(file_path, (int)(separator - file_path)) : String();
	const char *filename = separator ? separator + 1 : file_path;

	RegisterFont(Noesis::Uri(folder.get()), filename, 0, family, family, weight, stretch, style);
}

void NoesisFontProvider::ScanFolder(const Noesis::Uri& /*folder*/)
{}

Noesis::Ptr<Noesis::Stream> NoesisFontProvider::OpenFont(const Noesis::Uri& folder,
	const char* filename) const
{
	Noesis::FixedString<256> folder_path;
	folder.GetPath(folder_path);
	const char *folder_str = folder_path.Str();
	String disk_path = *folder_str ? String(folder_str) + "/" + filename : String(filename);

	auto it = buffers_.find(disk_path);
	if (it == buffers_.end())
	{
		String full_path = root_path_ + "/" + disk_path;
		Vector<uint8_t> bytes;
		if (!read_file_bytes(full_path, bytes))
		{
			Log::warning("NoesisGUI: failed to open font \"%s\"\n", full_path.get());
			return nullptr;
		}
		it = buffers_.append(std::move(disk_path), std::move(bytes));
	}
	const Vector<uint8_t>& buf = it->data;
	return Noesis::MakePtr<Noesis::MemoryStream>(buf.get(), (uint32_t)buf.size());
}

//////////////////////////////////////////////////////////////////////////
// NoesisXamlProvider
//////////////////////////////////////////////////////////////////////////

NoesisXamlProvider::NoesisXamlProvider(const char* root_path)
	: root_path_(root_path)
{}

Noesis::Ptr<Noesis::Stream> NoesisXamlProvider::LoadXaml(const Noesis::Uri& uri)
{
	Noesis::FixedString<512> uri_path;
	uri.GetPath(uri_path);
	String key(uri_path.Str());

	auto it = buffers_.find(key);
	if (it == buffers_.end())
	{
		String path = root_path_ + "/" + uri_path.Str();
		Vector<uint8_t> bytes;
		if (!read_file_bytes(path, bytes))
		{
			Log::warning("NoesisGUI: XAML not found \"%s\"\n", path.get());
			return nullptr;
		}
		it = buffers_.append(std::move(key), std::move(bytes));
	}
	const Vector<uint8_t>& buf = it->data;
	return Noesis::MakePtr<Noesis::MemoryStream>(buf.get(), (uint32_t)buf.size());
}

//////////////////////////////////////////////////////////////////////////
// NoesisTextureProvider
//////////////////////////////////////////////////////////////////////////

NoesisTextureProvider::NoesisTextureProvider(const char* root_path)
	: root_path_(root_path)
{}

Noesis::TextureInfo NoesisTextureProvider::GetTextureInfo(const Noesis::Uri& uri)
{
	ImagePtr image;
	if (!load_image_rgba8(uri, image))
		return {};
	return { (uint32_t)image->getWidth(), (uint32_t)image->getHeight() };
}

Noesis::Ptr<Noesis::Texture> NoesisTextureProvider::LoadTexture(const Noesis::Uri& uri,
	Noesis::RenderDevice* /*device*/)
{
	ImagePtr image;
	if (!load_image_rgba8(uri, image))
		return nullptr;

	bool has_alpha = true;
	if (image->getFormat() != Image::FORMAT_RGBA8)
	{
		if (image->getNumChannels() != 4)
			has_alpha = false;

		image->convertToFormat(Image::FORMAT_RGBA8);
	}

	TexturePtr tex = Texture::create();
	tex->create(image);
	return Noesis::MakePtr<NoesisTexture>(tex, has_alpha);
}

bool NoesisTextureProvider::load_image_rgba8(const Noesis::Uri& uri, ImagePtr& out_image)
{
	String path = root_path_ + "/" + uri.Str();
	Vector<uint8_t> bytes;
	if (!read_file_bytes(path, bytes))
	{
		Log::warning("NoesisGUI: texture not found \"%s\"\n", path.get());
		return false;
	}
	ImagePtr image = Image::create();
	if (!image->load(bytes.get(), (int)bytes.size()))
	{
		Log::warning("NoesisGUI: failed to decode texture \"%s\"\n", path.get());
		return false;
	}
	out_image = image;
	return true;
}
