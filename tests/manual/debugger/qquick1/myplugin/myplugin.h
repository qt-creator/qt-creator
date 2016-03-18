#pragma once

#include <qdeclarative.h>
#include <QDeclarativeExtensionPlugin>

class MyPlugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT

public:
    void registerTypes(const char *uri);
};
