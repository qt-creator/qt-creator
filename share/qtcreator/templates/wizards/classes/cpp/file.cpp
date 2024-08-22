%{JS: Cpp.licenseTemplate()}\
#include "%{JS: Util.relativeFilePath('%{Path}/%{HdrFileName}', '%{Path}' + '/' + Util.path('%{SrcFileName}'))}"

@if '%{IncludeQSharedData}'
#include <utility>

@endif
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

%{CN}::%{CN}(%{CN} &&rhs)
    : data{std::move(rhs.data)}
{

}

%{CN} &%{CN}::operator=(const %{CN} &rhs)
{
    if (this != &rhs)
        data = rhs.data;
    return *this;
}

%{CN} &%{CN}::operator=(%{CN} &&rhs)
{
    if (this != &rhs)
        data = std::move(rhs.data);
    return *this;
}

%{CN}::~%{CN}()
{

}
@endif
%{JS: Cpp.closeNamespaces('%{Class}')}\
