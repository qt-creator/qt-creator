#ifndef %ProjectName:u%_PLUGIN_H
#define %ProjectName:u%_PLUGIN_H

#include <QtDeclarative/QDeclarativeExtensionPlugin>

class %ProjectName%Plugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

#endif // %ProjectName:u%_PLUGIN_H
