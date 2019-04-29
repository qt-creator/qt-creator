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
%{JS: '%{PluginMethods}'.split('|').map(s => '\n' + s.replace(/([a-zA-Z0-9]+\()/, '%{CN}::$1') + '\n\u007B\n    static_assert(false, "You need to implement this function");\n\u007D').join('\n')}\

@endif
%{JS: Cpp.closeNamespaces('%{Class}')}\
