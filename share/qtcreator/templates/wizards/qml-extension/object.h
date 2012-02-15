#ifndef %ObjectName:u%_H
#define %ObjectName:u%_H

#include <QDeclarativeItem>

class %ObjectName% : public QDeclarativeItem
{
    Q_OBJECT
    Q_DISABLE_COPY(%ObjectName%)

public:
    %ObjectName%(QDeclarativeItem *parent = 0);
    ~%ObjectName%();
};

QML_DECLARE_TYPE(%ObjectName%)

#endif // %ObjectName:u%_H
