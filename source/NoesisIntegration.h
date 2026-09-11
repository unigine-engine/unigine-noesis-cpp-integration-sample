#pragma once

#include <UnigineEvent.h>
#include <UnigineInput.h>
#include <UnigineString.h>
#include <UnigineVector.h>
#include <UnigineWindowManager.h>

#include <NsCore/Ptr.h>
#include <NsGui/FontProperties.h>

#include "NoesisRenderDevice.h"

namespace Noesis { NS_INTERFACE IView; }

class NoesisView;
class ObjectNoesisGui;
class NoesisDataContext;
class NoesisFontProvider;

class NoesisIntegration final
{
public:
	NoesisIntegration();
	~NoesisIntegration();

	static NoesisIntegration *get() { return s_instance; }

	int init();
	int shutdown();

	void setApplicationResources(const char *xaml_path);
	Unigine::String getApplicationResources() const;

	void setDefaultFontSize(float size);
	float getDefaultFontSize() const;

	void setFontFallbacks(const char *fallbacks);
	Unigine::String getFontFallbacks() const;

	void registerFont(const char *file_path, const char *family,
		Noesis::FontWeight weight, Noesis::FontStretch stretch, Noesis::FontStyle style);

	void setGlyphCacheSize(int size);
	int getGlyphCacheSize() const;

	NoesisView *createView(const char *xaml_path);
	bool destroyView(NoesisView *view);

	ObjectNoesisGui *createObject(const char *xaml_path = "");
	bool destroyObject(ObjectNoesisGui *obj);

	NoesisDataContext *createDataContext();
	bool destroyDataContext(NoesisDataContext *ctx);

	// Internal: for ObjectNoesisGui initialization
	NoesisRenderDevice *getNoesisRenderDevice() const { return render_device_.GetPtr(); }

	// World UI registry — populated by ObjectNoesisGui ctor/dtor.
	void registerWorldView(ObjectNoesisGui *view);
	void unregisterWorldView(ObjectNoesisGui *view);

private:
	static NoesisIntegration *s_instance;

	void update_views();
	void render_views(const Unigine::EngineWindowViewportPtr &window);
	bool handle_mouse_input();
	bool handle_world_mouse_input();
	void handle_keyboard_input();

	void apply_font_fallbacks();
	void apply_font_defaults();

	Noesis::Ptr<NoesisRenderDevice> render_device_;
	Unigine::Vector<NoesisView *> active_views_;
	Unigine::Vector<ObjectNoesisGui *> world_views_;
	Unigine::Vector<ObjectNoesisGui *> created_objects_;
	Unigine::Vector<NoesisDataContext *> created_contexts_;
	ObjectNoesisGui *last_active_world_view_ = nullptr;
	Unigine::EngineWindowViewportPtr main_window_;

	Unigine::EventConnection update_connection_;
	Unigine::EventConnection render_connection_;

	bool left_mouse_pressed_ = false;
	bool right_mouse_pressed_ = false;
	bool middle_mouse_pressed_ = false;

	// Captured once in init() — restored every frame when UI is not capturing the mouse.
	Unigine::Input::MOUSE_HANDLE prev_mouse_handle_ = Unigine::Input::MOUSE_HANDLE_GRAB;

	bool initialized_ = false;

	Unigine::String application_resources_;
	float default_font_size_{14.0f};
	Unigine::String font_fallbacks_;
	int glyph_cache_size_{2048};
	NoesisFontProvider *font_provider_{nullptr}; 
};
