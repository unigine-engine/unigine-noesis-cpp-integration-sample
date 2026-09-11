#include "NoesisDataContext.h"

#include <NsCore/Boxing.h>
#include <NsCore/String.h>
#include <NsGui/Binding.h>
#include <NsGui/BaseCommand.h>

using namespace Unigine;

class NoesisDelegateCommand final : public Noesis::BaseCommand
{
public:
    explicit NoesisDelegateCommand(std::function<void()> callback)
        : callback_(std::move(callback)) {}

    bool CanExecute(Noesis::BaseComponent *) const override { return true; }
    void Execute(Noesis::BaseComponent *) const override { if (callback_) callback_(); }

    NS_IMPLEMENT_INLINE_REFLECTION(NoesisDelegateCommand, Noesis::BaseCommand, "Unigine.NoesisDelegateCommand")
    {}

private:
    std::function<void()> callback_;
};

// --- NoesisDataContextImpl ---

bool NoesisDataContextImpl::TryGet(const char *key, Noesis::Ptr<Noesis::BaseComponent> &item) const
{
    Noesis::Symbol sym(key);
    auto it = values_.Find(sym);
    if (it == values_.End())
        return false;
    item = it->value;
    return true;
}

bool NoesisDataContextImpl::TrySet(const char *key, Noesis::BaseComponent *item)
{
    Noesis::Symbol sym(key);
    auto it = values_.Find(sym);
    if (it == values_.End())
        return false;
    it->value = Noesis::Ptr<Noesis::BaseComponent>(item);
    notify_changed(sym);
    return true;
}

void NoesisDataContextImpl::set(Noesis::Symbol key, Noesis::Ptr<Noesis::BaseComponent> value)
{
    values_[key] = std::move(value);
    notify_changed(key);
}

void NoesisDataContextImpl::notify_changed(Noesis::Symbol key)
{
    changed_(this, Noesis::PropertyChangedEventArgs(key));
    changed_(this, Noesis::PropertyChangedEventArgs(Noesis::Binding::ItemNotifyName()));
}

bool NoesisDataContextImpl::has(Noesis::Symbol key) const
{
    return values_.Contains(key);
}

void NoesisDataContextImpl::remove(Noesis::Symbol key)
{
    values_.Erase(key);
}

void NoesisDataContextImpl::clear()
{
    values_.Clear();
}

// --- NoesisDataContext ---

NoesisDataContext::NoesisDataContext()
    : impl_(Noesis::MakePtr<NoesisDataContextImpl>())
{
    impl_->PropertyChanged() += Noesis::MakeDelegate(this, &NoesisDataContext::on_impl_changed);
}

NoesisDataContext::~NoesisDataContext()
{
    if (impl_)
        impl_->PropertyChanged() -= Noesis::MakeDelegate(this, &NoesisDataContext::on_impl_changed);
}

void NoesisDataContext::setFloat(const char *name, float value)
{
    impl_->set(Noesis::Symbol(name), Noesis::Boxing::Box<float>(value));
}

float NoesisDataContext::getFloat(const char *name) const
{
    Noesis::Ptr<Noesis::BaseComponent> item;
    if (!impl_->TryGet(name, item) || !item)
        return 0.0f;
    return Noesis::Boxing::Unbox<float>(item.GetPtr());
}

void NoesisDataContext::setInt(const char *name, int value)
{
    impl_->set(Noesis::Symbol(name), Noesis::Boxing::Box<int32_t>((int32_t)value));
}

int NoesisDataContext::getInt(const char *name) const
{
    Noesis::Ptr<Noesis::BaseComponent> item;
    if (!impl_->TryGet(name, item) || !item)
        return 0;
    return (int)Noesis::Boxing::Unbox<int32_t>(item.GetPtr());
}

void NoesisDataContext::setBool(const char *name, bool value)
{
    impl_->set(Noesis::Symbol(name), Noesis::Boxing::Box<bool>(value));
}

bool NoesisDataContext::getBool(const char *name) const
{
    Noesis::Ptr<Noesis::BaseComponent> item;
    if (!impl_->TryGet(name, item) || !item)
        return false;
    return Noesis::Boxing::Unbox<bool>(item.GetPtr());
}

void NoesisDataContext::setString(const char *name, const char *value)
{
    impl_->set(Noesis::Symbol(name), Noesis::Boxing::Box(value));
}

Unigine::String NoesisDataContext::getString(const char *name) const
{
    Noesis::Ptr<Noesis::BaseComponent> item;
    if (!impl_->TryGet(name, item) || !item)
        return {};
    return Noesis::Boxing::Unbox<Noesis::String>(item.GetPtr()).Str();
}

bool NoesisDataContext::hasProperty(const char *name) const
{
    return impl_->has(Noesis::Symbol(name));
}

void NoesisDataContext::removeProperty(const char *name)
{
    impl_->remove(Noesis::Symbol(name));
}

void NoesisDataContext::clear()
{
    impl_->clear();
}

void NoesisDataContext::on_impl_changed(Noesis::BaseComponent *,
    const Noesis::PropertyChangedEventArgs &args)
{
    event_changed_.run(this, args.propertyName.Str());
}

void NoesisDataContext::setCommand(const char *name, std::function<void()> callback)
{
    impl_->set(Noesis::Symbol(name), Noesis::MakePtr<NoesisDelegateCommand>(std::move(callback)));
}
