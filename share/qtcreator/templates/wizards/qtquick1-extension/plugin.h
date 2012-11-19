#ifndef %ProjectName:h%_PLUGIN_H
#define %ProjectName:h%_PLUGIN_H

#include <QDeclarativeExtensionPlugin>

class %ProjectName:s%Plugin : public QDeclarativeExtensionPlugin
{
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")
#endif

public:
    void registerTypes(const char *uri);
};

#endif // %ProjectName:h%_PLUGIN_H
