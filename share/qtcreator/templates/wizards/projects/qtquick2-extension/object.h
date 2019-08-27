%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{OBJECTGUARD}
#define %{OBJECTGUARD}
@endif

#include <QQuickItem>

class %{ObjectName} : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(%{ObjectName})

public:
    explicit %{ObjectName}(QQuickItem *parent = nullptr);
    ~%{ObjectName}() override;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // %{OBJECTGUARD}
@endif
