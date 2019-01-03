@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %ProjectName:h%_PLUGIN_H
#define %ProjectName:h%_PLUGIN_H
@endif

#include <QQmlExtensionPlugin>

class %ProjectName:s%Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // %ProjectName:h%_PLUGIN_H
@endif
