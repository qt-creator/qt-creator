#ifndef MYPLUGIN_H
#define MYPLUGIN_H

#include <QQmlExtensionPlugin>

class MyPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
public:
    void registerTypes(const char *uri);
};

#endif // MYPLUGIN_H
