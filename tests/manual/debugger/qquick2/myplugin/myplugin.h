#ifndef MYPLUGIN_H
#define MYPLUGIN_H

#include <qdeclarative.h>
#include <QDeclarativeExtensionPlugin>

class MyPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};

#endif // MYPLUGIN_H
