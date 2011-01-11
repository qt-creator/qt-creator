#ifndef %ProjectName:h%_PLUGIN_H
#define %ProjectName:h%_PLUGIN_H

#include <QtDeclarative/QDeclarativeExtensionPlugin>

class %ProjectName:s%Plugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

#endif // %ProjectName:h%_PLUGIN_H
