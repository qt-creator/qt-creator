%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{GUARD}
#define %{GUARD}
@endif

%{JS: QtSupport.qtIncludes([ 'QtGui/%{BaseClass}' ], [ 'QtWidgets/%{BaseClass}' ]) }\
%{JS: Cpp.openNamespaces('%{Class}')}\
@if %{GenerateForm}

@if ! %{JS: Cpp.hasNamespaces('%{Class}')}
QT_BEGIN_NAMESPACE
@endif
namespace Ui { class %{CN}; }
@if ! %{JS: Cpp.hasNamespaces('%{Class}')}
QT_END_NAMESPACE
@endif
@endif

class %{CN} : public %{BaseClass}
{
    Q_OBJECT

public:
    %{CN}(QWidget *parent = nullptr);
    ~%{CN}();
@if %{GenerateForm}

private:
    Ui::%{CN} *ui;
@endif
};
%{JS: Cpp.closeNamespaces('%{Class}')}\
@if ! '%{Cpp:PragmaOnce}'
#endif // %{GUARD}
@endif
