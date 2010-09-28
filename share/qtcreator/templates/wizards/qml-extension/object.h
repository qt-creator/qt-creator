#ifndef %ObjectName:u%_H
#define %ObjectName:u%_H

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QTimer>

class %ObjectName% : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(%ObjectName%)

public:
    %ObjectName%(QObject *parent = 0);
};

#endif // %ObjectName:u%_H
