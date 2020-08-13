%{Cpp:LicenseTemplate}\
#include "%{JS: Util.relativeFilePath('%{Path}/%{HdrFileName}', '%{Path}' + '/' + Util.path('%{SrcFileName}'))}"
@if %{GenerateForm} && %{JS: QtSupport.uiAsPointer() }
#include "%{UiHdrFileName}"
@endif
%{JS: Cpp.openNamespaces('%{Class}')}\

%{CN}::%{CN}(QWidget *parent)
    : %{BaseClass}(parent)
@if %{GenerateForm} && %{JS: QtSupport.uiAsPointer() }
    , ui(new Ui::%{CN})
@endif
{
@if %{GenerateForm}
@if %{JS: QtSupport.uiAsPointer() }
    ui->setupUi(this);
@elsif %{JS: QtSupport.uiAsMember() }
    ui.setupUi(this);
@else
    setupUi(this);
@endif
@endif
}

%{CN}::~%{CN}()
{
@if %{GenerateForm} && %{JS: QtSupport.uiAsPointer() }
    delete ui;
@endif
}

%{JS: Cpp.closeNamespaces('%{Class}')}\
