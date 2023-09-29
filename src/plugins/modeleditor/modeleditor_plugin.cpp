// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "modeleditor_plugin.h"

#include "jsextension.h"
#include "modeleditorfactory.h"
#include "modelsmanager.h"
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

void ModelEditorPlugin::initialize()
{
    d = new ModelEditorPluginPrivate;

    Core::JsExpander::registerGlobalObject<JsExtension>("Modeling");
}

void ModelEditorPlugin::extensionsInitialized()
{
    d->actionHandler.createActions();
    d->uiController.loadSettings();
}

ExtensionSystem::IPlugin::ShutdownFlag ModelEditorPlugin::aboutToShutdown()
{
    d->uiController.saveSettings();
    return SynchronousShutdown;
}

ModelsManager *ModelEditorPlugin::modelsManager()
{
    return &pluginInstance->d->modelsManager;
}

} // namespace Internal
} // namespace ModelEditor

