@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %ObjectName:u%_H
#define %ObjectName:u%_H
@endif

#include <QQuickItem>

class %ObjectName% : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(%ObjectName%)

public:
    explicit %ObjectName%(QQuickItem *parent = nullptr);
    ~%ObjectName%() override;
};

@if ! '%{Cpp:PragmaOnce}'
#endif // %ObjectName:u%_H
@endif
