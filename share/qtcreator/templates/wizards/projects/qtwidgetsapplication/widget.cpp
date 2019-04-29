%{Cpp:LicenseTemplate}\
#include "%{JS: Util.relativeFilePath('%{Path}/%{HdrFileName}', '%{Path}' + '/' + Util.path('%{SrcFileName}'))}"
@if %{GenerateForm}
#include "%{UiHdrFileName}"
@endif
%{JS: Cpp.openNamespaces('%{Class}')}\

%{CN}::%{CN}(QWidget *parent)
    : %{BaseClass}(parent)
@if %{GenerateForm}
    , ui(new Ui::%{CN})
@endif
{
@if %{GenerateForm}
    ui->setupUi(this);
@endif
}

%{CN}::~%{CN}()
{
@if %{GenerateForm}
    delete ui;
@endif
}

%{JS: Cpp.closeNamespaces('%{Class}')}\
