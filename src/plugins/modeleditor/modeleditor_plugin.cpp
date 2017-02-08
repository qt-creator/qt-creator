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

#include "modeleditorfactory.h"
#include "modelsmanager.h"
#include "settingscontroller.h"
#include "modeleditor_constants.h"
#include "uicontroller.h"
#include "jsextension.h"

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
#include <QMessageBox>
#include <QMainWindow>
#include <QMenu>
#include <QItemSelection>
#include <QClipboard>
#include <QFontDatabase>

#include <QtPlugin>

namespace ModelEditor {
namespace Internal {

ModelEditorPlugin *pluginInstance = 0;

class ModelEditorPlugin::ModelEditorPluginPrivate
{
public:
    ModelsManager *modelsManager = 0;
    UiController *uiController = 0;
    ModelEditorFactory *modelFactory = 0;
    SettingsController *settingsController = 0;
};

ModelEditorPlugin::ModelEditorPlugin()
    : ExtensionSystem::IPlugin(),
      d(new ModelEditorPluginPrivate)
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
    Q_UNUSED(arguments);
    Q_UNUSED(errorString);

    d->modelsManager = new ModelsManager(this);
    addAutoReleasedObject(d->modelsManager);

    d->uiController = new UiController(this);
    addAutoReleasedObject(d->uiController);

    d->modelFactory = new ModelEditorFactory(d->uiController, this);
    addAutoReleasedObject(d->modelFactory);

    d->settingsController = new SettingsController(this);
    addAutoReleasedObject(d->settingsController);

    Core::JsExpander::registerQObjectForJs(QLatin1String("Modeling"), new JsExtension(this));

    connect(d->settingsController, &SettingsController::saveSettings,
            d->uiController, &UiController::saveSettings);
    connect(d->settingsController, &SettingsController::loadSettings,
            d->uiController, &UiController::loadSettings);

    return true;
}

void ModelEditorPlugin::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // In the extensionsInitialized method, a plugin can be sure that all
    // plugins that depend on it are completely initialized.
    d->modelFactory->extensionsInitialized();
    d->settingsController->load(Core::ICore::settings());
}

ExtensionSystem::IPlugin::ShutdownFlag ModelEditorPlugin::aboutToShutdown()
{
    d->settingsController->save(Core::ICore::settings());
    QApplication::clipboard()->clear();
    return SynchronousShutdown;
}

ModelsManager *ModelEditorPlugin::modelsManager()
{
    return pluginInstance->d->modelsManager;
}

} // namespace Internal
} // namespace ModelEditor

