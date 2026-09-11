#pragma once

#include <UnigineEvent.h>
#include <UnigineString.h>

#include <NsCore/Ptr.h>
#include <functional>
#include <NsCore/BaseComponent.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/Delegate.h>
#include <NsGui/IDictionaryIndexer.h>
#include <NsGui/INotifyPropertyChanged.h>
#include <NsCore/HashMap.h>

// Internal Noesis-side object implementing IDictionaryIndexer + INotifyPropertyChanged.
// Passed directly to Noesis::FrameworkElement::SetDataContext().
class NoesisDataContextImpl final
    : public Noesis::BaseComponent
    , public Noesis::IDictionaryIndexer
    , public Noesis::INotifyPropertyChanged
{
public:
    NS_IMPLEMENT_INTERFACE_FIXUP

    bool TryGet(const char *key, Noesis::Ptr<Noesis::BaseComponent> &item) const override;
    bool TrySet(const char *key, Noesis::BaseComponent *item) override;

    Noesis::PropertyChangedEventHandler &PropertyChanged() override { return changed_; }

    void set(Noesis::Symbol key, Noesis::Ptr<Noesis::BaseComponent> value);
    bool has(Noesis::Symbol key) const;
    void remove(Noesis::Symbol key);
    void clear();

    NS_IMPLEMENT_INLINE_REFLECTION(NoesisDataContextImpl, Noesis::BaseComponent, "Unigine.NoesisDataContext")
    {
        NsImpl<Noesis::IDictionaryIndexer>();
        NsImpl<Noesis::INotifyPropertyChanged>();
    }

private:
    void notify_changed(Noesis::Symbol key);

    Noesis::HashMap<Noesis::Symbol, Noesis::Ptr<Noesis::BaseComponent>> values_;
    Noesis::PropertyChangedEventHandler changed_;
};

class NoesisDataContext final
{
public:
    NoesisDataContext();
    ~NoesisDataContext();

    void setFloat(const char *name, float value);
    float getFloat(const char *name) const;
    void setInt(const char *name, int value);
    int getInt(const char *name) const;
    void setBool(const char *name, bool value);
    bool getBool(const char *name) const;
    void setString(const char *name, const char *value);
    Unigine::String getString(const char *name) const;
    void setCommand(const char *name, std::function<void()> callback);

    bool hasProperty(const char *name) const;
    void removeProperty(const char *name);
    void clear();

    Unigine::Event<NoesisDataContext *, const char *> &getEventPropertyChanged()
    {
        return event_changed_;
    }

    NoesisDataContextImpl *getImpl() const { return impl_.GetPtr(); }

private:
    void on_impl_changed(Noesis::BaseComponent *sender, const Noesis::PropertyChangedEventArgs &args);

    Noesis::Ptr<NoesisDataContextImpl> impl_;
    Unigine::EventInvoker<NoesisDataContext *, const char *> event_changed_;
};
