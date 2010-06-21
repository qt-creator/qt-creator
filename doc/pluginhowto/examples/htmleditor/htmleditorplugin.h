#ifndef HTMLEDITOR_PLUGIN_H
#define HTMLEDITOR_PLUGIN_H

#include "extensionsystem/iplugin.h"
#include "coreplugin/uniqueidmanager.h"

class HTMLEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    HTMLEditorPlugin();
    ~HTMLEditorPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
};

#endif // HTMLEDITOR_PLUGIN_H

