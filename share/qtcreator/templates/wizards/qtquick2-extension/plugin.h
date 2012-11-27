#ifndef %ProjectName:h%_PLUGIN_H
#define %ProjectName:h%_PLUGIN_H

#include <QQmlExtensionPlugin>

class %ProjectName:s%Plugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri);
};

#endif // %ProjectName:h%_PLUGIN_H
