%{Cpp:LicenseTemplate}\
#include "%{JS: Util.relativeFilePath('%{Path}/%{HdrFileName}', '%{Path}' + '/' + Util.path('%{SrcFileName}'))}"
%{JS: Cpp.openNamespaces('%{Class}')}\

@if ! %{IsQtPlugin}
%{CN}::%{CN}()
{
}
@else
%{CN}::%{CN}(QObject *parent)
    : %{BaseClassName}(parent)
{
}
%{JS: '%{PluginMethods}'.split('|').map(function(s) { return '\\n' + s.replace(/([a-zA-Z0-9]+\\()/, '%{CN}::$1') + '\\n\{\\n    static_assert(false, "You need to implement this function");\\n\}'; \}).join('\\n')}\

@endif
%{JS: Cpp.closeNamespaces('%{Class}')}\
