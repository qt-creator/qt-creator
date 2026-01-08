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

@if %{GenerateForm} && %{JS: QtSupport.uiAsPointer() }
%{CN}::~%{CN}()
{
    delete ui;
}
@else
%{CN}::~%{CN}() = default;
@endif

%{JS: Cpp.closeNamespaces('%{Class}')}\
