#include "ObjectNoesisGui.h"
#include "NoesisIntegration.h"

#include <UnigineXml.h>
#include <UnigineMathLib.h>
#include <UnigineGame.h>
#include <UnigineEngine.h>
#include <UnigineLog.h>
#include <UniginePlayers.h>

#include <NsGui/IntegrationAPI.h>
#include <NsGui/IRenderer.h>
#include <NsGui/FrameworkElement.h>
#include <NsGui/Uri.h>

using namespace Unigine;
using namespace Unigine::Math;

ObjectNoesisGui::ObjectNoesisGui()
{
	update_bounds();
	updateSurfaces();
	if (NoesisIntegration *integration = NoesisIntegration::get())
		integration->registerWorldView(this);
}

ObjectNoesisGui::ObjectNoesisGui(void *object)
	: ObjectExternBase(object)
{
	update_bounds();
	if (NoesisIntegration *integration = NoesisIntegration::get())
		integration->registerWorldView(this);
}

ObjectNoesisGui::~ObjectNoesisGui()
{
	if (NoesisIntegration *integration = NoesisIntegration::get())
		integration->unregisterWorldView(this);
	if (noesis_view_)
	{
		noesis_view_->GetRenderer()->Shutdown();
		noesis_view_.Reset();
	}
}

void ObjectNoesisGui::setPhysicalSize(float width, float height)
{
	physical_width_ = width;
	physical_height_ = height;
	update_bounds();
}

void ObjectNoesisGui::setBillboard(bool billboard)
{
	if (billboard_ == billboard)
		return;
	billboard_ = billboard;
	update_bounds();
	updateSurfaces();
	if (surface_material_)
		surface_material_->setState("billboard", billboard_ ? 1 : 0);
}

void ObjectNoesisGui::preRender(float ifps)
{
	UNIGINE_UNUSED(ifps);
	initialize_render();
	if (view_ready_)
		noesis_view_->Update(Game::getTime());
}

void ObjectNoesisGui::render(Render::PASS pass, int surface)
{
	if (pass != Render::PASS_AMBIENT || !view_ready_)
		return;

	{
		RenderState::saveState();
		RenderState::clearStates();
		render_target_->enable();
		RenderState::clearBuffer(RenderState::BUFFER_COLOR, vec4(0.0f, 0.0f, 0.0f, 0.0f));

		Noesis::IRenderer *renderer = noesis_view_->GetRenderer();
		renderer->UpdateRenderTree();
		renderer->RenderOffscreen();
		renderer->Render();

		render_target_->disable();
		RenderState::restoreState();
	}

	color_texture_->createMipmaps();

	Renderer::setShaderParameters(pass, getObject(), surface, false);
	ShaderPtr shader = RenderState::getShader();
	if (!shader)
		return;

	RenderState::setTexture(RenderState::BIND_FRAGMENT, 0, color_texture_);
	RenderState::setPolygonCull(RenderState::CULL_NONE);
	// Noesis outputs premultiplied alpha.
	RenderState::setBlendFunc(
		RenderState::BLEND_ONE, RenderState::BLEND_ONE_MINUS_SRC_ALPHA,
		RenderState::BLEND_OP_ADD);
	RenderState::setDepthFunc(depth_test_ ? RenderState::DEPTH_GEQUAL : RenderState::DEPTH_NONE);
	RenderState::setDepthWrite(false);
	RenderState::flushStates();

	const float half_w = physical_width_ * 0.5f;
	const float half_h = physical_height_ * 0.5f;
	color_texture_->render2D(-half_w, -half_h, half_w, half_h);
}

int ObjectNoesisGui::loadWorld(const Ptr<Xml> &xml)
{
	xaml_path_ = xml->getArg("xaml");
	physical_width_ = xml->getFloatArg("physical_width", 1.0f);
	physical_height_ = xml->getFloatArg("physical_height", 1.0f);
	screen_width_ = xml->getIntArg("screen_width", 1024);
	screen_height_ = xml->getIntArg("screen_height", 1024);
	depth_test_ = xml->getBoolArg("depth_test", true);
	billboard_ = xml->getBoolArg("billboard", false);
	polygon_offset_ = xml->getFloatArg("polygon_offset", 0.0f);
	control_distance_ = xml->getFloatArg("control_distance", 1.0f);
	mouse_mode_ = xml->getIntArg("mouse_mode", MOUSE_STANDARD);
	update_bounds();
	return 1;
}

int ObjectNoesisGui::getIntersection(const Vec3 &p0_, const Vec3 &p1_, Vec3 *ret_point,
	vec3 *ret_normal, vec4 *ret_texcoord, int *ret_index, int *ret_instance, int surface)
{
	UNIGINE_UNUSED(surface);

	const vec4 plane(0.0f, 0.0f, 1.0f, 0.0f);
	vec3 p0;
	vec3 p1;
	Mat4 billboard_transform;
	bool use_billboard = false;

	if (billboard_)
	{
		PlayerPtr player = Engine::get()->getMainPlayer();
		if (!player)
			return 0;
		NodePtr node = getNode();
		if (!node)
			return 0;
		billboard_transform = node->getIWorldTransform()
			* Math::translate(node->getWorldTransform().getColumn3(3))
			* Math::rotation(player->getWorldTransform());
		Mat4 itransform = Math::inverse(billboard_transform);
		p0 = vec3(itransform * p0_);
		p1 = vec3(itransform * p1_);
		use_billboard = true;
	}
	else
	{
		p0 = vec3(p0_);
		p1 = vec3(p1_);
	}

	float d0 = dot(plane, p0);
	float d1 = dot(plane, p1);
	float denom = d0 - d1;
	if (Math::abs(denom) < Consts::EPS)
		return 0;
	float dist = d0 / denom;
	if (dist < 0.0f || dist > 1.0f)
		return 0;

	vec3 point = p0 + (p1 - p0) * dist;
	float x = point.x / physical_width_ + 0.5f;
	float y = point.y / physical_height_ + 0.5f;
	if (x < 0.0f || x > 1.0f || y < 0.0f || y > 1.0f)
		return 0;

	if (use_billboard)
	{
		if (ret_point)
			ret_point->set(billboard_transform * Vec3(point));
		if (ret_normal)
			ret_normal->set(vec3(billboard_transform.getColumn3(2)));
	}
	else
	{
		if (ret_point)
			ret_point->set(Vec3(point));
		if (ret_normal)
			ret_normal->set(0.0f, 0.0f, 1.0f);
	}
	if (ret_texcoord)
		ret_texcoord->set(x, y, 0.0f, 0.0f);
	if (ret_index)
		*ret_index = 0;
	if (ret_instance)
		*ret_instance = -1;
	return 1;
}

void ObjectNoesisGui::setMouse(const Vec3 &p0, const Vec3 &p1, int buttons)
{
	manual_p0_ = p0;
	manual_p1_ = p1;
	manual_buttons_ = buttons;
}

bool ObjectNoesisGui::forwardMouse(int px, int py, int wheel, bool left, bool right, bool middle)
{
	if (!view_ready_)
		return false;

	Noesis::IView *iview = noesis_view_.GetPtr();
	bool consumed = iview->MouseMove(px, py);

	if (wheel != 0)
		consumed |= iview->MouseWheel(px, py, wheel);
	if (left && !prev_left_pressed_)
		consumed |= iview->MouseButtonDown(px, py, Noesis::MouseButton_Left);
	else if (!left && prev_left_pressed_)
		consumed |= iview->MouseButtonUp(px, py, Noesis::MouseButton_Left);
	if (right && !prev_right_pressed_)
		consumed |= iview->MouseButtonDown(px, py, Noesis::MouseButton_Right);
	else if (!right && prev_right_pressed_)
		consumed |= iview->MouseButtonUp(px, py, Noesis::MouseButton_Right);
	if (middle && !prev_middle_pressed_)
		consumed |= iview->MouseButtonDown(px, py, Noesis::MouseButton_Middle);
	else if (!middle && prev_middle_pressed_)
		consumed |= iview->MouseButtonUp(px, py, Noesis::MouseButton_Middle);

	prev_left_pressed_ = left;
	prev_right_pressed_ = right;
	prev_middle_pressed_ = middle;

	return consumed;
}

void ObjectNoesisGui::forwardMouseLeave()
{
	if (!view_ready_)
		return;

	if (!prev_left_pressed_ && !prev_right_pressed_ && !prev_middle_pressed_)
		return;

	Noesis::IView *iview = noesis_view_.GetPtr();

	if (prev_left_pressed_)
		iview->MouseButtonUp(-1, -1, Noesis::MouseButton_Left);
	if (prev_right_pressed_)
		iview->MouseButtonUp(-1, -1, Noesis::MouseButton_Right);
	if (prev_middle_pressed_)
		iview->MouseButtonUp(-1, -1, Noesis::MouseButton_Middle);

	prev_left_pressed_ = false;
	prev_right_pressed_ = false;
	prev_middle_pressed_ = false;
}

void ObjectNoesisGui::forwardKeyDown(Noesis::Key key)
{
	if (view_ready_)
		noesis_view_->KeyDown(key);
}

void ObjectNoesisGui::forwardKeyUp(Noesis::Key key)
{
	if (view_ready_)
		noesis_view_->KeyUp(key);
}

void ObjectNoesisGui::forwardChar(uint32_t code)
{
	if (view_ready_)
		noesis_view_->Char(code);
}

int ObjectNoesisGui::saveWorld(const Ptr<Xml> &xml)
{
	xml->setArg("xaml", xaml_path_.get());
	xml->setFloatArg("physical_width", physical_width_);
	xml->setFloatArg("physical_height", physical_height_);
	xml->setIntArg("screen_width", screen_width_);
	xml->setIntArg("screen_height", screen_height_);
	xml->setBoolArg("depth_test", depth_test_);
	xml->setBoolArg("billboard", billboard_);
	xml->setFloatArg("polygon_offset", polygon_offset_);
	xml->setFloatArg("control_distance", control_distance_);
	xml->setIntArg("mouse_mode", mouse_mode_);
	return 1;
}

void ObjectNoesisGui::initialize_render()
{
	if (view_ready_ || xaml_path_.empty())
		return;

	Noesis::Ptr<Noesis::FrameworkElement> root = Noesis::GUI::LoadXaml<Noesis::FrameworkElement>(xaml_path_.get());
	if (!root)
	{
		Log::error("ObjectNoesisGui: failed to load XAML '%s'\n", xaml_path_.get());
		return;
	}

	noesis_view_ = Noesis::GUI::CreateView(root);
	noesis_view_->SetFlags(Noesis::RenderFlags_PPAA);
	noesis_view_->SetTessellationMaxPixelError(Noesis::TessellationMaxPixelError::HighQuality());
	noesis_view_->SetSize(screen_width_, screen_height_);

	NoesisIntegration *integration = NoesisIntegration::get();
	if (!integration)
		return;

	noesis_view_->GetRenderer()->Init(integration->getNoesisRenderDevice());

	color_texture_ = Texture::create();
	color_texture_->create2D(screen_width_, screen_height_, Texture::FORMAT_RGBA8,
		Texture::FORMAT_USAGE_RENDER | Texture::FORMAT_MIPMAPS
		| Texture::SAMPLER_FILTER_TRILINEAR | Texture::SAMPLER_ANISOTROPY_16
		| Texture::SAMPLER_WRAP_CLAMP);

	render_target_ = RenderTarget::create();
	render_target_->bindColorTexture(0, color_texture_);

	surface_material_ = Materials::findMaterialByPath("noesis/materials/noesis_gui_object.basemat");
	if (surface_material_)
	{
		surface_material_ = surface_material_->inherit();
		surface_material_->setState("billboard", billboard_ ? 1 : 0);
		getObject()->setMaterial(surface_material_, 0);
	}
	else
	{
		Log::error("ObjectNoesisGui: material not found: noesis/materials/noesis_gui_object.basemat\n");
	}

	if (data_ctx_)
		noesis_view_->GetContent()->SetDataContext(data_ctx_->getImpl());

	view_ready_ = true;
}

void ObjectNoesisGui::setDataContext(NoesisDataContext *ctx)
{
	data_ctx_ = ctx;
	if (view_ready_ && noesis_view_)
		noesis_view_->GetContent()->SetDataContext(data_ctx_ ? data_ctx_->getImpl() : nullptr);
}

void ObjectNoesisGui::update_bounds()
{
	float half_w = physical_width_ * 0.5f;
	float half_h = physical_height_ * 0.5f;
	if (billboard_)
	{
		float radius = ::sqrtf(half_w * half_w + half_h * half_h);
		bound_box_ = BoundBox(vec3(-radius), vec3(radius));
		bound_sphere_ = BoundSphere(vec3(0.0f), radius);
	}
	else
	{
		bound_box_ = BoundBox(vec3(-half_w, -half_h, -0.001f), vec3(half_w, half_h, 0.001f));
		bound_sphere_ = BoundSphere(vec3(0.0f), ::sqrtf(half_w * half_w + half_h * half_h));
	}
}
