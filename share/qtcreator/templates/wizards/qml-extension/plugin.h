#ifndef %ProjectName:u%_H
#define %ProjectName:u%_H

#include <QtDeclarative/QDeclarativeExtensionPlugin>

class %ProjectName% : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

#endif // %ProjectName:u%_H
