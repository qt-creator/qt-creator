// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeleditor_plugin.h"

#include "jsextension.h"
#include "modeleditor_constants.h"
#include "modeleditorfactory.h"
#include "modeleditor_global.h"
#include "modelsmanager.h"
#include "settingscontroller.h"
#include "uicontroller.h"
#include "actionhandler.h"

#include <qmt/infrastructure/uid.h>
#include <qmt/model/mdiagram.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>

#include <QAction>
#include <QApplication>
#include <QItemSelection>
#include <QClipboard>

namespace ModelEditor {
namespace Internal {

ModelEditorPlugin *pluginInstance = nullptr;

class ModelEditorPluginPrivate final
{
public:
    ModelsManager modelsManager;
    UiController uiController;
    ActionHandler actionHandler;
    ModelEditorFactory modelFactory{&uiController, &actionHandler};
    SettingsController settingsController;
};

ModelEditorPlugin::ModelEditorPlugin()
{
    pluginInstance = this;
    qRegisterMetaType<QItemSelection>("QItemSelection");
    qRegisterMetaType<qmt::Uid>("qmt::Uid");
    qRegisterMetaType<qmt::MDiagram *>();
    qRegisterMetaType<const qmt::MDiagram *>();
}

ModelEditorPlugin::~ModelEditorPlugin()
{
    delete d;
}

bool ModelEditorPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)
    d = new ModelEditorPluginPrivate;

    Core::JsExpander::registerGlobalObject<JsExtension>("Modeling");

    connect(&d->settingsController, &SettingsController::saveSettings,
            &d->uiController, &UiController::saveSettings);
    connect(&d->settingsController, &SettingsController::loadSettings,
            &d->uiController, &UiController::loadSettings);

    return true;
}

void ModelEditorPlugin::extensionsInitialized()
{
    d->actionHandler.createActions();
    d->settingsController.load(Core::ICore::settings());
}

ExtensionSystem::IPlugin::ShutdownFlag ModelEditorPlugin::aboutToShutdown()
{
    d->settingsController.save(Core::ICore::settings());
    return SynchronousShutdown;
}

ModelsManager *ModelEditorPlugin::modelsManager()
{
    return &pluginInstance->d->modelsManager;
}

} // namespace Internal
} // namespace ModelEditor

