%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{OBJECTGUARD}
#define %{OBJECTGUARD}
@endif

#include <QtQuick/QQuickPaintedItem>

class %{ObjectName} : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY(%{ObjectName})
public:
    explicit %{ObjectName}(QQuickItem *parent = nullptr);
    void paint(QPainter *painter) override;
    ~%{ObjectName}() override;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // %{OBJECTGUARD}
@endif
