%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{GUARD}
#define %{GUARD}
@endif
@if %{IsShared}

#include "%{GlobalHdrFileName}"
@elsif %{IsQtPlugin}

#include <%{BaseClassName}>
@endif
%{JS: Cpp.openNamespaces('%{Class}')}\

@if %{IsShared}
class %{LibraryExport} %{CN}
{
public:
    %{CN}();
};
@elsif %{IsStatic}
class %{CN}
{
public:
    %{CN}();
};
@else
class %{CN} : public %{BaseClassName}
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID %{PluginInterface}_iid FILE "%{PluginJsonFile}")

public:
    explicit %{CN}(QObject *parent = nullptr);

private:
%{JS: '%{PluginMethods}'.split('|').map(function(s) { return '    ' + s + ' override;'; \}).join('\\n')}
};
@endif
%{JS: Cpp.closeNamespaces('%{Class}')}\
@if ! '%{Cpp:PragmaOnce}'

#endif // %{GUARD}
@endif
