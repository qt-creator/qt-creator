/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "modeleditor_plugin.h"

#include "jsextension.h"
#include "modeleditor_constants.h"
#include "modeleditorfactory.h"
#include "modeleditor_global.h"
#include "modelsmanager.h"
#include "settingscontroller.h"
#include "uicontroller.h"
#include "actionhandler.h"

#include "qmt/infrastructure/uid.h"

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
    QApplication::clipboard()->clear();
    return SynchronousShutdown;
}

ModelsManager *ModelEditorPlugin::modelsManager()
{
    return &pluginInstance->d->modelsManager;
}

} // namespace Internal
} // namespace ModelEditor

