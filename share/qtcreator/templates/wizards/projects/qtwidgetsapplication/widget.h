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

namespace Ui { class %{CN}; }
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
