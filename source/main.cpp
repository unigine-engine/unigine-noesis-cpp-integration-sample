#if defined(_WIN32)
// D3D12 Agility SDK redistributable, deployed under the app's bin/D3D12 folder.
// The path is resolved relative to the executable, which lives in bin/.
// Windows-only: the Linux build renders through Vulkan.
#	define D3D12_AGILITY_SDK_PATH "D3D12"
#endif

#include <UnigineInit.h>
#include <UnigineEngine.h>
#include <UnigineLogic.h>
#include <UnigineWorld.h>
#include <UnigineInput.h>
#include <UnigineLog.h>
#include <UnigineGame.h>
#include <UnigineMathLib.h>
#include <UnigineNodes.h>
#include <UnigineVisualizer.h>
#include <UniginePlayers.h>
#include <UnigineString.h>

#include "NoesisIntegration.h"
#include "NoesisView.h"
#include "ObjectNoesisGui.h"
#include "NoesisDataContext.h"

#include <NsGui/FontProperties.h>

using namespace Unigine;

namespace
{
NoesisIntegration *noesis_gui = nullptr;
NoesisView *noesis_view_overlay_1 = nullptr;
NoesisView *noesis_view_overlay_2 = nullptr;
ObjectNoesisGui *noesis_object_gui = nullptr;
NoesisDataContext *data_ctx = nullptr;
EventConnection data_ctx_connection;

static const float TIME_PRESET_ANGLES[] = {45.0f, 90.0f, 150.0f};
} // namespace

class AppSystemLogic final : public SystemLogic
{
public:
	int init() override
	{
		noesis_gui = new NoesisIntegration();
		bool is_inited = noesis_gui->init();

		if (is_inited)
		{
			noesis_gui->setDefaultFontSize(16.0f);
			noesis_gui->setGlyphCacheSize(2048);

			noesis_gui->registerFont("ui/fonts/Muli-Regular.ttf", "Muli",
									 Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);
			noesis_gui->registerFont("ui/fonts/CourierPrime-Regular.ttf", "Courier Prime",
									 Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);
			noesis_gui->registerFont("ui/fonts/Caladea-Regular.ttf", "Caladea",
									 Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);

			
			noesis_gui->registerFont("ui/themes/noesis/Fonts/PT Root UI_Regular.otf", "PT Root UI",
									 Noesis::FontWeight_Normal, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);
			noesis_gui->registerFont("ui/themes/noesis/Fonts/PT Root UI_Bold.otf", "PT Root UI",
									 Noesis::FontWeight_Bold, Noesis::FontStretch_Normal, Noesis::FontStyle_Normal);

			noesis_gui->setFontFallbacks("ui/themes/noesis/Fonts/#PT Root UI");
			noesis_gui->setApplicationResources("ui/themes/noesis/NoesisTheme.DarkBlue.xaml");
		}

		return is_inited;
	}

	int shutdown() override
	{
		if (noesis_gui)
		{
			noesis_gui->shutdown();
			delete noesis_gui;
			noesis_gui = nullptr;
		}
		return 1;
	}
};

class AppWorldLogic final : public WorldLogic
{
public:
	int init() override
	{
		if (!noesis_gui)
			return 1;

		data_ctx = noesis_gui->createDataContext();
		data_ctx->setFloat("sun_angle_x", 45.0f);
		data_ctx->setInt("time_preset", 0);
		// Matches the ComboBoxItem tags in text_font.xaml: a FontFamily that arrives through a
		// binding has no source XAML to resolve against, so the folder is relative to data root.
		data_ctx->setString("font_family", "ui/fonts/#Muli");
		data_ctx->getEventPropertyChanged().connect(data_ctx_connection,
			[](NoesisDataContext *ctx, const char *name)
			{
				if (String::equal(name, "sun_angle_x"))
				{
					float angle_x = ctx->getFloat("sun_angle_x");
					NodePtr sun_node = World::getNodeByName("sun");
					if (sun_node)
						sun_node->setWorldTransform(Math::Mat4(Math::rotateX(angle_x)));
				}
				else if (String::equal(name, "time_preset"))
				{
					int preset = ctx->getInt("time_preset");
					if (preset >= 0 && preset < 3)
						ctx->setFloat("sun_angle_x", TIME_PRESET_ANGLES[preset]);
				}
			}
		);

		// Trigger initial sun rotation
		NodePtr sun_node = World::getNodeByName("sun");
		if (sun_node)
			sun_node->setWorldTransform(Math::Mat4(Math::rotateX(45.0f)));

		// World object with gui (3D surface).
		noesis_object_gui = noesis_gui->createObject("ui/world.xaml");
		if (noesis_object_gui)
		{
			noesis_object_gui->getNode()->setWorldTransform(Math::Mat4(Math::rotateX(60.0f)));
			noesis_object_gui->setPhysicalSize(2.0f, 2.0f);
			noesis_object_gui->setScreenSize(1024, 1024);
			noesis_object_gui->setDepthTest(true);
			noesis_object_gui->setBillboard(false);
			noesis_object_gui->setControlDistance(5.0f);
			noesis_object_gui->setDataContext(data_ctx);
		}

		// Overlay pinned to a sub-rect (viewport-style).
		noesis_view_overlay_1 = noesis_gui->createView("ui/overlay.xaml");
		if (noesis_view_overlay_1)
		{
			noesis_view_overlay_1->setRect(20, 20, 500, 300);
			noesis_view_overlay_1->setDataContext(data_ctx);
		}

		// Full-window overlay (font showcase).
		noesis_view_overlay_2 = noesis_gui->createView("ui/text_font.xaml");
		if (noesis_view_overlay_2)
			noesis_view_overlay_2->setDataContext(data_ctx);

		return 1;
	}

	int shutdown() override
	{
		if (noesis_gui)
		{
			if (noesis_view_overlay_1)
			{
				noesis_gui->destroyView(noesis_view_overlay_1);
				noesis_view_overlay_1 = nullptr;
			}

			if (noesis_view_overlay_2)
			{
				noesis_gui->destroyView(noesis_view_overlay_2);
				noesis_view_overlay_2 = nullptr;
			}

			if (noesis_object_gui)
			{
				noesis_gui->destroyObject(noesis_object_gui);
				noesis_object_gui = nullptr;
			}

			if (data_ctx)
			{
				data_ctx_connection.disconnect();
				noesis_gui->destroyDataContext(data_ctx);
				data_ctx = nullptr;
			}
		}
		return 1;
	}
};

int main(int argc, char **argv)
{
	Engine::InitParameters init_params;
	init_params.window_title = "UNIGINE Engine: NoesisGUI sample";

	EnginePtr engine(init_params, argc, argv);
	Engine::get()->setBackgroundUpdate(Engine::BACKGROUND_UPDATE_RENDER_NON_MINIMIZED);

	AppSystemLogic system_logic;
	AppWorldLogic world_logic;
	engine->main(&system_logic, &world_logic);
	return 0;
}
