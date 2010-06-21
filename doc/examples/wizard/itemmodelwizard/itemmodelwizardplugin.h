#ifndef ITEMMODELWIZARD_PLUGIN_H
#define ITEMMODELWIZARD_PLUGIN_H

#include <extensionsystem/iplugin.h>

class ItemModelWizardPlugin : public ExtensionSystem::IPlugin
{
public:
    ItemModelWizardPlugin();
    ~ItemModelWizardPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
};

#endif // ITEMMODELWIZARD_PLUGIN_H

