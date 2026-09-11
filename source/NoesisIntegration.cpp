#include "NoesisIntegration.h"

#include <cstring>
#include "NoesisView.h"
#include "NoesisRenderDevice.h"
#include "NoesisProviders.h"
#include "ObjectNoesisGui.h"
#include "NoesisDataContext.h"

#include <UnigineEngine.h>
#include <UnigineRender.h>
#include <UnigineObjects.h>
#include <UnigineGame.h>
#include <UnigineInput.h>
#include <UnigineConsole.h>
#include <UnigineLog.h>
#include <UnigineStreams.h>
#include <UnigineString.h>
#include <UnigineWindowManager.h>
#include <UnigineProfiler.h>
#include <UnigineMathLib.h>
#include <UnigineNode.h>
#include <UniginePlayers.h>
#include <UnigineVisualizer.h>

#include <NsCore/Log.h>
#include <NsGui/IntegrationAPI.h>
#include <NsGui/IView.h>
#include <NsGui/IRenderer.h>
#include <NsGui/FrameworkElement.h>
#include <NsGui/InputEnums.h>
#include <NsGui/Uri.h>

extern "C" void NsRegisterReflectionAppInteractivity();
extern "C" void NsInitPackageAppInteractivity();
extern "C" void NsShutdownPackageAppInteractivity();

using namespace Unigine;

namespace
{

void noesis_log(const char *file, uint32_t line, uint32_t level, const char *channel,
	const char *message)
{
	UNIGINE_UNUSED(file);
	UNIGINE_UNUSED(line);
	UNIGINE_UNUSED(channel);
	if (level >= 4)
		Log::error("[Noesis] %s\n", message);
	else if (level == 3)
		Log::warning("[Noesis] %s\n", message);
	else
		Log::message("[Noesis] %s\n", message);
}

struct KeyMapping { Input::KEY unigine_key; Noesis::Key noesis_key; };
static const KeyMapping KEY_MAP[] = {
	{ Input::KEY_LEFT,      Noesis::Key_Left   },
	{ Input::KEY_RIGHT,     Noesis::Key_Right  },
	{ Input::KEY_UP,        Noesis::Key_Up     },
	{ Input::KEY_DOWN,      Noesis::Key_Down   },
	{ Input::KEY_ENTER,     Noesis::Key_Return },
	{ Input::KEY_ESC,       Noesis::Key_Escape },
	{ Input::KEY_TAB,       Noesis::Key_Tab    },
	{ Input::KEY_BACKSPACE, Noesis::Key_Back   },
	{ Input::KEY_DELETE,    Noesis::Key_Delete },
	{ Input::KEY_HOME,      Noesis::Key_Home   },
	{ Input::KEY_END,       Noesis::Key_End    },
	{ Input::KEY_SPACE,     Noesis::Key_Space  },
};

} // namespace

NoesisIntegration *NoesisIntegration::s_instance = nullptr;

NoesisIntegration::NoesisIntegration()
{
	s_instance = this;
}

NoesisIntegration::~NoesisIntegration()
{
	if (s_instance == this)
		s_instance = nullptr;
}

int NoesisIntegration::init()
{
	ObjectExternBase::addClassID<ObjectNoesisGui>(ObjectNoesisGui::CLASS_ID);

	Noesis::SetLogHandler(noesis_log);
	Noesis::GUI::SetLicense("", "");
	Noesis::GUI::Init();
	NsRegisterReflectionAppInteractivity();
	NsInitPackageAppInteractivity();

	const String data_path(String::normalizePath(Engine::get()->getDataPath()));
	Noesis::GUI::SetXamlProvider(Noesis::MakePtr<NoesisXamlProvider>(data_path));
	Noesis::GUI::SetTextureProvider(Noesis::MakePtr<NoesisTextureProvider>(data_path));

	auto font_provider = Noesis::MakePtr<NoesisFontProvider>(data_path);
	font_provider_ = font_provider.GetPtr();
	Noesis::GUI::SetFontProvider(std::move(font_provider));

	render_device_ = Noesis::MakePtr<NoesisRenderDevice>(
		Texture::FORMAT_RGBA8, Texture::FORMAT_D24S8);

	initialized_ = true;

	prev_mouse_handle_ = Input::getMouseHandle();

	setGlyphCacheSize(glyph_cache_size_);
	apply_font_defaults();

	Engine::get()->getEventEndInputUpdate().connect(update_connection_,
		[this]() { update_views(); });

	main_window_ = WindowManager::getMainWindow();
	if (main_window_)
	{
		main_window_->getEventFuncBeginRenderGui().connect(render_connection_,
			[this]() { render_views(main_window_); });
	}
	else
	{
		Log::error("NoesisIntegration: no main window — render will not work\n");
	}

	return 1;
}

int NoesisIntegration::shutdown()
{
	update_connection_.disconnect();
	render_connection_.disconnect();
	main_window_ = nullptr;

	for (NoesisView *view : active_views_)
		delete view;
	active_views_.clear();

	for (ObjectNoesisGui *obj : created_objects_)
		delete obj;
	created_objects_.clear();

	for (NoesisDataContext *ctx : created_contexts_)
		delete ctx;
	created_contexts_.clear();

	render_device_.Reset();

	if (initialized_)
	{
		Input::setMouseHandle(prev_mouse_handle_);
		NsShutdownPackageAppInteractivity();
		Noesis::GUI::Shutdown();
		initialized_ = false;
	}
	return 1;
}

void NoesisIntegration::setApplicationResources(const char *xaml_path)
{
	application_resources_ = xaml_path;
	if (initialized_ && !application_resources_.empty())
		Noesis::GUI::LoadApplicationResources(Noesis::Uri(application_resources_.get()));
}

String NoesisIntegration::getApplicationResources() const { return application_resources_; }

void NoesisIntegration::setDefaultFontSize(float size)
{
	default_font_size_ = size;
	if (initialized_)
		apply_font_defaults();
}

float NoesisIntegration::getDefaultFontSize() const { return default_font_size_; }

void NoesisIntegration::setFontFallbacks(const char *fallbacks)
{
	font_fallbacks_ = fallbacks;
	if (initialized_)
		apply_font_fallbacks();
}

String NoesisIntegration::getFontFallbacks() const { return font_fallbacks_; }

void NoesisIntegration::registerFont(const char *file_path, const char *family,
	Noesis::FontWeight weight, Noesis::FontStretch stretch, Noesis::FontStyle style)
{
	if (!font_provider_)
	{
		Log::warning("NoesisIntegration::registerFont: called before init()\n");
		return;
	}
	font_provider_->register_face(file_path, family, weight, stretch, style);
}

void NoesisIntegration::setGlyphCacheSize(int size)
{
	glyph_cache_size_ = size;
	if (initialized_)
	{
		render_device_->SetGlyphCacheWidth(size);
		render_device_->SetGlyphCacheHeight(size);
	}
}

int NoesisIntegration::getGlyphCacheSize() const { return glyph_cache_size_; }

void NoesisIntegration::apply_font_fallbacks()
{
	Vector<String> names;
	String remaining = font_fallbacks_;
	while (!remaining.empty())
	{
		int comma = remaining.find(',');
		if (comma < 0)
		{
			names.append(remaining);
			break;
		}
		names.append(String(remaining.get(), comma));
		remaining = String(remaining.get() + comma + 1);
	}

	Vector<const char *> ptrs;
	ptrs.resize(names.size());
	for (int i = 0; i < names.size(); ++i)
		ptrs[i] = names[i].get();

	if (!ptrs.empty())
		Noesis::GUI::SetFontFallbacks(ptrs.get(), (uint32_t)ptrs.size());
}

void NoesisIntegration::apply_font_defaults()
{
	Noesis::GUI::SetFontDefaultProperties(default_font_size_,
		Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);
}

NoesisView *NoesisIntegration::createView(const char *xaml_path)
{
	Noesis::Ptr<Noesis::FrameworkElement> root =
		Noesis::GUI::LoadXaml<Noesis::FrameworkElement>(xaml_path);
	if (!root)
	{
		Log::error("NoesisIntegration: failed to load XAML '%s'\n", xaml_path);
		return nullptr;
	}

	Noesis::Ptr<Noesis::IView> iview = Noesis::GUI::CreateView(root);
	iview->SetFlags(Noesis::RenderFlags_PPAA);

	if (main_window_)
	{
		Math::ivec2 size = main_window_->getClientRenderSize();
		iview->SetSize(size.x, size.y);
	}

	iview->GetRenderer()->Init(render_device_.GetPtr());

	NoesisView *view = new NoesisView(std::move(iview), xaml_path);
	active_views_.append(view);
	return view;
}

bool NoesisIntegration::destroyView(NoesisView *view)
{
	for (int i = 0; i < active_views_.size(); ++i)
	{
		if (active_views_[i] == view)
		{
			delete active_views_[i];
			active_views_.remove(i);
			return true;
		}
	}
	return false;
}

ObjectNoesisGui *NoesisIntegration::createObject(const char *xaml_path)
{
	auto *obj = new ObjectNoesisGui();
	if (xaml_path && *xaml_path)
		obj->setXaml(xaml_path);
	created_objects_.append(obj);
	return obj;
}

bool NoesisIntegration::destroyObject(ObjectNoesisGui *obj)
{
	if (!obj)
		return false;
	int idx = created_objects_.findIndex(obj);
	if (idx < 0)
		return false;
	created_objects_.remove(idx);
	delete obj;
	return true;
}

NoesisDataContext *NoesisIntegration::createDataContext()
{
	auto *ctx = new NoesisDataContext();
	created_contexts_.append(ctx);
	return ctx;
}

bool NoesisIntegration::destroyDataContext(NoesisDataContext *ctx)
{
	if (!ctx)
		return false;
	int idx = created_contexts_.findIndex(ctx);
	if (idx < 0)
		return false;
	created_contexts_.remove(idx);
	delete ctx;
	return true;
}

void NoesisIntegration::registerWorldView(ObjectNoesisGui *view)
{
	if (view && world_views_.findIndex(view) < 0)
		world_views_.append(view);
}

void NoesisIntegration::unregisterWorldView(ObjectNoesisGui *view)
{
	int index = world_views_.findIndex(view);
	if (index >= 0)
		world_views_.remove(index);
	if (last_active_world_view_ == view)
		last_active_world_view_ = nullptr;
}

void NoesisIntegration::update_views()
{
	const bool grab = Input::isMouseGrab();

	bool want_mouse = false;
	if (!grab)
	{
		want_mouse |= handle_mouse_input();
		want_mouse |= handle_world_mouse_input();
	}
	handle_keyboard_input();

	double time = Game::getTime();
	for (NoesisView *view : active_views_)
	{
		if (view->isEnabled())
			view->getNoesisView()->Update(time);
	}

	if (!grab)
		Input::setMouseHandle(want_mouse ? Input::MOUSE_HANDLE_USER : prev_mouse_handle_);
}

bool NoesisIntegration::handle_world_mouse_input()
{
	if (world_views_.empty() || Console::isActive())
		return false;

	PlayerPtr player = Engine::get()->getMainPlayer();
	if (!player)
		return false;

	Math::ivec2 mouse_pos = Input::getMousePosition();
	Math::Vec3 std_p0, std_p1;
	player->getDirectionFromMainWindow(std_p0, std_p1, mouse_pos.x, mouse_pos.y);

	bool std_left   = Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT);
	bool std_right  = Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT);
	bool std_middle = Input::isMouseButtonPressed(Input::MOUSE_BUTTON_MIDDLE);
	int  std_wheel  = Input::getMouseWheel();

	ObjectNoesisGui *hit_view = nullptr;
	float hit_distance = Math::Consts::INF;
	Math::vec4 hit_uv;
	for (ObjectNoesisGui *wv : world_views_)
	{
		if (!wv->isViewReady())
			continue;

		Math::Vec3 wp0, wp1;
		if (wv->getMouseMode() == ObjectNoesisGui::MOUSE_VIRTUAL)
		{
			wp0 = wv->getManualP0();
			wp1 = wv->getManualP1();
		}
		else
		{
			wp0 = std_p0;
			Math::Vec3 dir_norm = Math::normalize(std_p1 - std_p0);
			wp1 = wp0 + dir_norm * wv->getControlDistance();
		}

		NodePtr node = wv->getNode();
		if (!node)
			continue;
		const Math::Mat4 itransform = node->getIWorldTransform();
		Math::Vec3 lp0 = itransform * wp0;
		Math::Vec3 lp1 = itransform * wp1;

		Math::Vec3 local_hit;
		Math::vec4 texcoord;
		if (!wv->getIntersection(lp0, lp1, &local_hit, nullptr, &texcoord, nullptr, nullptr, 0))
			continue;

		Math::Vec3 world_hit = node->getWorldTransform() * local_hit;
		Visualizer::renderPoint3D(world_hit, 0.01f, Math::vec4_green, false, 0.0f, false);

		float distance = (float)Math::length(world_hit - wp0);
		if (distance < hit_distance)
		{
			hit_distance = distance;
			hit_view = wv;
			hit_uv = texcoord;
		}
	}

	bool consumed = false;
	for (ObjectNoesisGui *wv : world_views_)
	{
		if (wv != hit_view)
		{
			wv->forwardMouseLeave();
			continue;
		}

		int px = Math::ftoi(hit_uv.x * Math::itof(wv->getScreenWidth()));
		int py = Math::ftoi((1.0f - hit_uv.y) * Math::itof(wv->getScreenHeight()));

		bool left, right, middle;
		int wheel;
		if (wv->getMouseMode() == ObjectNoesisGui::MOUSE_VIRTUAL)
		{
			int btn = wv->getManualButtons();
			left   = (btn & 1) != 0;
			right  = (btn & 2) != 0;
			middle = (btn & 4) != 0;
			wheel = 0;
		}
		else
		{
			left   = std_left;
			right  = std_right;
			middle = std_middle;
			wheel  = std_wheel;
		}

		consumed |= wv->forwardMouse(px, py, wheel, left, right, middle);
		last_active_world_view_ = wv;
	}
	return consumed;
}

void NoesisIntegration::render_views(const EngineWindowViewportPtr &window)
{
	if (active_views_.empty())
		return;

	UNIGINE_PROFILER_FUNCTION_GPU;

	const Math::ivec2 client = window->getClientRenderSize();

	for (NoesisView *view : active_views_)
	{
		if (!view->isEnabled())
			continue;

		const Math::ivec4 rect = view->isRectFullWindow()
			? Math::ivec4(0, 0, client.x, client.y)
			: view->getRect();

		Noesis::IView *iview = view->getNoesisView();
		iview->SetSize(rect.z, rect.w);

		Noesis::IRenderer *renderer = iview->GetRenderer();
		renderer->UpdateRenderTree();
		renderer->RenderOffscreen();

		RenderState::saveState();
		RenderState::setViewport(rect.x, rect.y, rect.z, rect.w);
		RenderState::setScissorTest((float)rect.x, (float)rect.y, (float)rect.z, (float)rect.w);

		renderer->Render();

		RenderState::restoreState();
	}
}

bool NoesisIntegration::handle_mouse_input()
{
	if (Console::isActive())
		return false;

	const Math::ivec2 mouse_pos = Input::getMousePosition();
	const Math::ivec2 client_pos = main_window_ ? main_window_->getClientPosition() : Math::ivec2(0, 0);
	const Math::ivec2 client_size = main_window_ ? main_window_->getClientRenderSize() : Math::ivec2(0, 0);
	const int cx = mouse_pos.x - client_pos.x;
	const int cy = mouse_pos.y - client_pos.y;

	const int wheel = Input::getMouseWheel();
	const bool left   = Input::isMouseButtonPressed(Input::MOUSE_BUTTON_LEFT);
	const bool right  = Input::isMouseButtonPressed(Input::MOUSE_BUTTON_RIGHT);
	const bool middle = Input::isMouseButtonPressed(Input::MOUSE_BUTTON_MIDDLE);

	bool consumed = false;
	for (NoesisView *view : active_views_)
	{
		if (!view->isEnabled())
			continue;

		Noesis::IView *iview = view->getNoesisView();

		const Math::ivec4 rect = view->isRectFullWindow()
			? Math::ivec4(0, 0, client_size.x, client_size.y)
			: view->getRect();
		const bool inside = cx >= rect.x && cx < rect.x + rect.z
		                 && cy >= rect.y && cy < rect.y + rect.w;

		if (!inside)
		{
			// Move cursor out of view bounds to clear hover state.
			iview->MouseMove(-1, -1);
			continue;
		}

		const int vx = cx - rect.x;
		const int vy = cy - rect.y;

		consumed |= iview->MouseMove(vx, vy);

		if (wheel != 0)
			consumed |= iview->MouseWheel(vx, vy, wheel);

		if (left && !left_mouse_pressed_)
			consumed |= iview->MouseButtonDown(vx, vy, Noesis::MouseButton_Left);
		else if (!left && left_mouse_pressed_)
			consumed |= iview->MouseButtonUp(vx, vy, Noesis::MouseButton_Left);

		if (right && !right_mouse_pressed_)
			consumed |= iview->MouseButtonDown(vx, vy, Noesis::MouseButton_Right);
		else if (!right && right_mouse_pressed_)
			consumed |= iview->MouseButtonUp(vx, vy, Noesis::MouseButton_Right);

		if (middle && !middle_mouse_pressed_)
			consumed |= iview->MouseButtonDown(vx, vy, Noesis::MouseButton_Middle);
		else if (!middle && middle_mouse_pressed_)
			consumed |= iview->MouseButtonUp(vx, vy, Noesis::MouseButton_Middle);
	}

	left_mouse_pressed_   = left;
	right_mouse_pressed_  = right;
	middle_mouse_pressed_ = middle;
	return consumed;
}

void NoesisIntegration::handle_keyboard_input()
{
	if (Console::isActive())
		return;

	for (const KeyMapping &m : KEY_MAP)
	{
		bool pressed = Input::isKeyPressed(m.unigine_key);
		bool up = Input::isKeyUp(m.unigine_key);
		if (!pressed && !up)
			continue;

		for (NoesisView *view : active_views_)
		{
			if (!view->isEnabled())
				continue;
			Noesis::IView *iview = view->getNoesisView();
			if (pressed)
				iview->KeyDown(m.noesis_key);
			else
				iview->KeyUp(m.noesis_key);
		}

		if (last_active_world_view_)
		{
			if (pressed)
				last_active_world_view_->forwardKeyDown(m.noesis_key);
			else
				last_active_world_view_->forwardKeyUp(m.noesis_key);
		}
	}
}
