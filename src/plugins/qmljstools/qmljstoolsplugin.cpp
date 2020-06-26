/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmljstoolsplugin.h"
#include "qmljsmodelmanager.h"
#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"
#include "qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmljsbundleprovider.h"

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

    QAction resetCodeModelAction{QmlJSToolsPlugin::tr("Reset Code Model"), nullptr};

    LocatorData locatorData;
    FunctionFilter functionFilter{&locatorData};
    QmlJSCodeStyleSettingsPage codeStyleSettingsPage;
    BasicBundleProvider basicBundleProvider;
};

QmlJSToolsPlugin::~QmlJSToolsPlugin()
{
    delete d;
}

bool QmlJSToolsPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)

    d = new QmlJSToolsPluginPrivate;

    return true;
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
    menu->setTitle(QmlJSToolsPlugin::tr("&QML/JS"));
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
