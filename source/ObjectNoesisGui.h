#pragma once

#include "NoesisDataContext.h"

#include <UnigineObjects.h>
#include <UnigineString.h>
#include <UnigineMathLibBounds.h>
#include <UnigineMaterials.h>
#include <UnigineTextures.h>
#include <UnigineRender.h>

#include <NsCore/Ptr.h>
#include <NsGui/IView.h>
#include <NsGui/InputEnums.h>

class ObjectNoesisGui final : public Unigine::ObjectExternBase
{
public:
	static constexpr int CLASS_ID = 1919;

	enum MOUSE_MODE
	{
		MOUSE_STANDARD = 0,
		MOUSE_VIRTUAL = 1,
	};

	ObjectNoesisGui();
	ObjectNoesisGui(void *object);
	~ObjectNoesisGui() override;

	// ObjectExternBase interface
	int getClassID() override { return CLASS_ID; }
	int getNumSurfaces() override { return 1; }
	const char *getSurfaceName(int surface) override { return "gui"; }

	const Unigine::Math::BoundBox &getBoundBox(int surface) override { return bound_box_; }
	const Unigine::Math::BoundSphere &getBoundSphere(int surface) override { return bound_sphere_; }
	const Unigine::Math::BoundBox &getBoundBox() override { return bound_box_; }
	const Unigine::Math::BoundSphere &getBoundSphere() override { return bound_sphere_; }

	bool hasRender() override { return true; }

	void preRender(float ifps) override;
	void render(Unigine::Render::PASS pass, int surface) override;

	int loadWorld(const Unigine::Ptr<Unigine::Xml> &xml) override;
	int saveWorld(const Unigine::Ptr<Unigine::Xml> &xml) override;

	// Ray-quad intersection in node-local space. Caller transforms world ray via getIWorldTransform.
	int getIntersection(const Unigine::Math::Vec3 &p0, const Unigine::Math::Vec3 &p1,
		Unigine::Math::Vec3 *ret_point, Unigine::Math::vec3 *ret_normal,
		Unigine::Math::vec4 *ret_texcoord, int *ret_index, int *ret_instance,
		int surface) override;

	void setXaml(const char *xaml) { xaml_path_ = xaml; }
	Unigine::String getXaml() const { return xaml_path_; }

	void setPhysicalSize(float w, float h);
	float getPhysicalWidth() const { return physical_width_; }
	float getPhysicalHeight() const { return physical_height_; }

	void setScreenSize(int w, int h) { screen_width_ = w; screen_height_ = h; }
	int getScreenWidth() const { return screen_width_; }
	int getScreenHeight() const { return screen_height_; }

	void setDepthTest(bool test) { depth_test_ = test; }
	bool isDepthTest() const { return depth_test_; }

	void setBillboard(bool billboard);
	bool isBillboard() const { return billboard_; }

	void setControlDistance(float distance) { control_distance_ = distance; }
	float getControlDistance() const { return control_distance_; }

	void setMouseMode(int mode) { mouse_mode_ = mode; }
	int getMouseMode() const { return mouse_mode_; }

	void setMouse(const Unigine::Math::Vec3 &p0, const Unigine::Math::Vec3 &p1, int buttons);

	void setDataContext(NoesisDataContext *ctx);
	NoesisDataContext *getDataContext() const { return data_ctx_; }

	// Internal only
	Noesis::IView *getNoesisView() const { return noesis_view_.GetPtr(); }
	bool isViewReady() const { return view_ready_; }

	const Unigine::Math::Vec3 &getManualP0() const { return manual_p0_; }
	const Unigine::Math::Vec3 &getManualP1() const { return manual_p1_; }
	int getManualButtons() const { return manual_buttons_; }

	// Returns true if Noesis consumed any event (cursor over an interactive element).
	bool forwardMouse(int px, int py, int wheel, bool left, bool right, bool middle);
	void forwardMouseLeave();
	void forwardKeyDown(Noesis::Key key);
	void forwardKeyUp(Noesis::Key key);
	void forwardChar(uint32_t code);

private:
	void initialize_render();
	void update_bounds();

	Unigine::String xaml_path_;
	float physical_width_ = 1.0f;
	float physical_height_ = 1.0f;
	int screen_width_ = 1024;
	int screen_height_ = 1024;
	bool depth_test_ = true;
	bool billboard_ = false;
	float polygon_offset_ = 0.0f;
	float control_distance_ = 1.0f;
	int mouse_mode_ = MOUSE_STANDARD;

	Unigine::Math::Vec3 manual_p0_;
	Unigine::Math::Vec3 manual_p1_;
	int manual_buttons_ = 0;

	bool prev_left_pressed_ = false;
	bool prev_right_pressed_ = false;
	bool prev_middle_pressed_ = false;

	Unigine::Math::BoundBox bound_box_;
	Unigine::Math::BoundSphere bound_sphere_;

	Noesis::Ptr<Noesis::IView> noesis_view_;
	Unigine::TexturePtr color_texture_;
	Unigine::RenderTargetPtr render_target_;
	Unigine::MaterialPtr surface_material_;
	bool view_ready_ = false;

	NoesisDataContext *data_ctx_ = nullptr; // non-owning
};
