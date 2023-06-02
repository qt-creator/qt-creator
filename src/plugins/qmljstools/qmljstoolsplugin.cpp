// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsbundleprovider.h"
#include "qmljscodestylesettingspage.h"
#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"
#include "qmljsmodelmanager.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolsplugin.h"
#include "qmljstoolssettings.h"
#include "qmljstoolstr.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <QMenu>

using namespace Core;

namespace QmlJSTools {
namespace Internal {

enum { debug = 0 };

class QmlJSToolsPluginPrivate : public QObject
{
public:
    QmlJSToolsPluginPrivate();

    QmlJSToolsSettings settings;
    ModelManager modelManager;

    QAction resetCodeModelAction{Tr::tr("Reset Code Model"), nullptr};

    LocatorData locatorData;
    QmlJSFunctionsFilter functionsFilter{&locatorData};
    QmlJSCodeStyleSettingsPage codeStyleSettingsPage;
    BasicBundleProvider basicBundleProvider;
};

QmlJSToolsPlugin::~QmlJSToolsPlugin()
{
    delete d;
}

void QmlJSToolsPlugin::initialize()
{
    d = new QmlJSToolsPluginPrivate;
}

QmlJSToolsPluginPrivate::QmlJSToolsPluginPrivate()
{
//    Core::VcsManager *vcsManager = Core::VcsManager::instance();
//    Core::DocumentManager *documentManager = Core::DocumentManager::instance();
//    connect(vcsManager, &Core::VcsManager::repositoryChanged,
//            &d->modelManager, &ModelManager::updateModifiedSourceFiles);
//    connect(documentManager, &DocumentManager::filesChangedInternally,
//            &d->modelManager, &ModelManager::updateSourceFiles);

    // Menus
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mqmljstools = ActionManager::createMenu(Constants::M_TOOLS_QMLJS);
    QMenu *menu = mqmljstools->menu();
    menu->setTitle(Tr::tr("&QML/JS"));
    menu->setEnabled(true);
    mtools->addMenu(mqmljstools);

    // Update context in global context
    Command *cmd = ActionManager::registerAction(
                &resetCodeModelAction, Constants::RESET_CODEMODEL);
    connect(&resetCodeModelAction, &QAction::triggered,
            &modelManager, &ModelManager::resetCodeModel);
    mqmljstools->addAction(cmd);

    // Watch task progress
    connect(ProgressManager::instance(), &ProgressManager::taskStarted, this,
            [this](Utils::Id type) {
                  if (type == QmlJS::Constants::TASK_INDEX)
                      resetCodeModelAction.setEnabled(false);
            });

    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            [this](Utils::Id type) {
                  if (type == QmlJS::Constants::TASK_INDEX)
                      resetCodeModelAction.setEnabled(true);
            });
}

void QmlJSToolsPlugin::extensionsInitialized()
{
    d->modelManager.delayedInitialization();
}

} // Internal
} // QmlJSTools
