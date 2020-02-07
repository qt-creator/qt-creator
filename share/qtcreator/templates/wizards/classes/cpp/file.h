%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{GUARD}
#define %{GUARD}
@endif

%{JS: Cpp.includeStatement('%{Base}', Util.preferredSuffix('text/x-c++hdr'), ['QObject', 'QWidget', 'QMainWindow', 'QQuickItem', 'QSharedData'], '%{TargetPath}')}\
%{JS: QtSupport.qtIncludes([ ( '%{IncludeQObject}' )          ? 'QtCore/%{IncludeQObject}'                 : '',
                             ( '%{IncludeQWidget}' )          ? 'QtGui/%{IncludeQWidget}'                  : '',
                             ( '%{IncludeQMainWindow}' )      ? 'QtGui/%{IncludeQMainWindow}'              : '',
                             ( '%{IncludeQDeclarativeItem}' ) ? 'QtDeclarative/%{IncludeQDeclarativeItem}' : '',
                             ( '%{IncludeQSharedData}' )      ? 'QtCore/QSharedDataPointer'                : '' ],
                           [ ( '%{IncludeQObject}' )          ? 'QtCore/%{IncludeQObject}'                 : '',
                             ( '%{IncludeQWidget}' )          ? 'QtWidgets/%{IncludeQWidget}'              : '',
                             ( '%{IncludeQMainWindow}' )      ? 'QtWidgets/%{IncludeQMainWindow}'          : '',
                             ( '%{IncludeQDeclarativeItem}' ) ? 'QtQuick1/%{IncludeQDeclarativeItem}'      : '',
                             ( '%{IncludeQQuickItem}' )       ? 'QtDeclarative/%{IncludeQQuickItem}'       : '',
                             ( '%{IncludeQSharedData}' )      ? 'QtCore/QSharedDataPointer'                : '' ])}\
%{JS: Cpp.openNamespaces('%{Class}')}
@if '%{IncludeQSharedData}'
class %{CN}Data;

@endif
@if '%{Base}'
class %{CN} : public %{Base}
@else
class %{CN}
@endif
{
@if '%{AddQObjectMacro}'
     Q_OBJECT
@endif
public:
@if '%{Base}' === 'QObject'
    explicit %{CN}(QObject *parent = nullptr);
@elsif '%{Base}' === 'QWidget' || '%{Base}' === 'QMainWindow'
    explicit %{CN}(QWidget *parent = nullptr);
@else
    %{CN}();
@endif
@if '%{IncludeQSharedData}'
    %{CN}(const %{CN} &);
    %{CN} &operator=(const %{CN} &);
    ~%{CN}();
@endif
@if %{isQObject}

@if %{QtKeywordsEnabled}
signals:
@else
Q_SIGNALS:
@endif

@endif
@if '%{IncludeQSharedData}'

private:
    QSharedDataPointer<%{CN}Data> data;
@endif
};
%{JS: Cpp.closeNamespaces('%{Class}')}
@if ! '%{Cpp:PragmaOnce}'
#endif // %{GUARD}
@endif
