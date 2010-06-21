#include "itemmodelwizardplugin.h"
#include "modelclasswizard.h"
#include <QApplication>
#include <QtPlugin>
#include <QStringList>

ItemModelWizardPlugin::ItemModelWizardPlugin()
{
    // Do nothing
}

ItemModelWizardPlugin::~ItemModelWizardPlugin()
{
    // Do notning
}

void ItemModelWizardPlugin::extensionsInitialized()
{
    // Do nothing
}

bool ItemModelWizardPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);
    Core::BaseFileWizardParameters params;
    params.setKind(Core::IWizard::ClassWizard);
    params.setIcon(qApp->windowIcon());
    params.setDescription("Generates an item-model class");
    params.setName("Item Model");
    params.setCategory("FooCompany");
    params.setTrCategory(tr("FooCompany"));
    addAutoReleasedObject(new ModelClassWizard(params, this));
    return true;

}

void ItemModelWizardPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(ItemModelWizardPlugin)
