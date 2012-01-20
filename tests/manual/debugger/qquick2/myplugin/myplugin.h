#ifndef MYPLUGIN_H
#define MYPLUGIN_H

#include <QtDeclarative/qdeclarative.h>
#include <QtDeclarative/QDeclarativeExtensionPlugin>

class MyPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

#endif // MYPLUGIN_H
