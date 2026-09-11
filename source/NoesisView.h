#pragma once

#include "NoesisDataContext.h"

#include <UnigineString.h>
#include <UnigineMathLib.h>

#include <NsCore/Ptr.h>

namespace Noesis { NS_INTERFACE IView; }

class NoesisView final
{
public:
    explicit NoesisView(Noesis::Ptr<Noesis::IView> iview, const char *xaml_path);
    ~NoesisView();

    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Viewport rect in client pixels. Input is clipped to this rect.
    void setRect(int x, int y, int width, int height);
    void setRectFullWindow(); // auto-fit to main window client area
    bool isRectFullWindow() const { return rect_full_window_; }
    Unigine::Math::ivec4 getRect() const { return rect_; } // (x, y, w, h); (0,0,0,0) when full-window

    Unigine::String getXamlPath() const;

    void setDataContext(NoesisDataContext *ctx);
    NoesisDataContext *getDataContext() const { return data_ctx_; }

    Noesis::IView *getNoesisView() const { return iview_.GetPtr(); }

private:
    Noesis::Ptr<Noesis::IView> iview_;
    Unigine::String xaml_path_;
    Unigine::Math::ivec4 rect_{0, 0, 0, 0};
    bool rect_full_window_ = true;
    bool enabled_ = true;
    NoesisDataContext *data_ctx_ = nullptr; // non-owning
};
