#include "NoesisView.h"

#include <NsGui/IView.h>
#include <NsGui/IRenderer.h>
#include <NsGui/FrameworkElement.h>

using namespace Unigine;

NoesisView::NoesisView(Noesis::Ptr<Noesis::IView> iview, const char *xaml_path)
	: iview_(std::move(iview))
	, xaml_path_(xaml_path)
{}

NoesisView::~NoesisView()
{
	if (iview_)
		iview_->GetRenderer()->Shutdown();
}

void NoesisView::setEnabled(bool enabled) { enabled_ = enabled; }
bool NoesisView::isEnabled() const { return enabled_; }

void NoesisView::setRect(int x, int y, int width, int height)
{
	rect_ = Math::ivec4(x, y, width, height);
	rect_full_window_ = false;
}

void NoesisView::setRectFullWindow()
{
	rect_full_window_ = true;
	rect_ = Math::ivec4(0, 0, 0, 0);
}

String NoesisView::getXamlPath() const { return xaml_path_; }

void NoesisView::setDataContext(NoesisDataContext *ctx)
{
	data_ctx_ = ctx;
	if (iview_)
	{
		Noesis::FrameworkElement *root = iview_->GetContent();
		if (root)
			root->SetDataContext(data_ctx_ ? data_ctx_->getImpl() : nullptr);
	}
}
