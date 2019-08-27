%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{PLUGINGUARD}
#define %{PLUGINGUARD}
@endif

#include <QQmlExtensionPlugin>

class %{PluginName} : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // %{PLUGINGUARD}
@endif
