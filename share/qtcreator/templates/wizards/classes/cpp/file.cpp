%{JS: Cpp.licenseTemplate()}\
#include "%{JS: Util.relativeFilePath('%{Path}/%{HdrFileName}', '%{Path}' + '/' + Util.path('%{SrcFileName}'))}"
%{JS: Cpp.openNamespaces('%{Class}')}
@if '%{IncludeQSharedData}'
class %{CN}Data : public QSharedData
{
public:

};

@endif
@if '%{Base}' === 'QObject'
%{CN}::%{CN}(QObject *parent)
    : QObject{parent}%{JS: ('%{SharedDataInit}') ? ', %{SharedDataInit}' : ''}
@elsif '%{Base}' === 'QWidget' || '%{Base}' === 'QMainWindow'
%{CN}::%{CN}(QWidget *parent)
    : %{Base}{parent}%{JS: ('%{SharedDataInit}') ? ', %{SharedDataInit}' : ''}
@elsif %{JS: Cpp.hasQObjectParent('%{Base}')}
%{CN}::%{CN}(QObject *parent)
    : %{Base}{parent}%{JS: ('%{SharedDataInit}') ? ', %{SharedDataInit}' : ''}
@else
%{CN}::%{CN}()%{JS: ('%{SharedDataInit}') ? ' : %{SharedDataInit}' : ''}
@endif
{

}
@if '%{IncludeQSharedData}'

%{CN}::%{CN}(const %{CN} &rhs)
    : data{rhs.data}
{

}

%{CN} &%{CN}::operator=(const %{CN} &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

%{CN}::~%{CN}()
{

}
@endif
%{JS: Cpp.closeNamespaces('%{Class}')}\
