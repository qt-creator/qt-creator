#ifndef %ObjectName:u%_H
#define %ObjectName:u%_H

#include <QQuickItem>

class %ObjectName% : public QQuickItem
{
    Q_OBJECT
    Q_DISABLE_COPY(%ObjectName%)

public:
    %ObjectName%(QQuickItem *parent = nullptr);
    ~%ObjectName%();
};

#endif // %ObjectName:u%_H
