/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljstoolsplugin.h"
#include "qmljsmodelmanager.h"
#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"
#include "qmljscodestylesettingspage.h"
#include "qmljstoolsconstants.h"
#include "qmljstoolssettings.h"
#include "qmlconsolemanager.h"
#include "qmljsbundleprovider.h"

#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QtPlugin>
#include <QMenu>

using namespace Core;
using namespace QmlJSTools;
using namespace QmlJSTools::Internal;

enum { debug = 0 };

QmlJSToolsPlugin *QmlJSToolsPlugin::m_instance = 0;

QmlJSToolsPlugin::QmlJSToolsPlugin()
    : m_modelManager(0)
{
    m_instance = this;
}

QmlJSToolsPlugin::~QmlJSToolsPlugin()
{
    m_instance = 0;
    m_modelManager = 0; // deleted automatically
    m_consoleManager = 0; // deleted automatically
}

bool QmlJSToolsPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments)
    Q_UNUSED(error)

    Utils::MimeDatabase::addMimeTypes(QLatin1String(":/qmljstools/QmlJSTools.mimetypes.xml"));

    m_settings = new QmlJSToolsSettings(this); // force registration of qmljstools settings

    // Objects
    m_modelManager = new ModelManager(this);
    m_consoleManager = new QmlConsoleManager(this);

//    VCSManager *vcsManager = core->vcsManager();
//    DocumentManager *fileManager = core->fileManager();
//    connect(vcsManager, SIGNAL(repositoryChanged(QString)),
//            m_modelManager, SLOT(updateModifiedSourceFiles()));
//    connect(fileManager, SIGNAL(filesChangedInternally(QStringList)),
//            m_modelManager, SLOT(updateSourceFiles(QStringList)));

    LocatorData *locatorData = new LocatorData;
    addAutoReleasedObject(locatorData);
    addAutoReleasedObject(new FunctionFilter(locatorData));
    addAutoReleasedObject(new QmlJSCodeStyleSettingsPage);
    addAutoReleasedObject(new BasicBundleProvider);

    // Menus
    ActionContainer *mtools = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ActionContainer *mqmljstools = ActionManager::createMenu(Constants::M_TOOLS_QMLJS);
    QMenu *menu = mqmljstools->menu();
    menu->setTitle(tr("&QML/JS"));
    menu->setEnabled(true);
    mtools->addMenu(mqmljstools);

    // Update context in global context
    m_resetCodeModelAction = new QAction(tr("Reset Code Model"), this);
    Command *cmd = ActionManager::registerAction(
                m_resetCodeModelAction, Constants::RESET_CODEMODEL);
    connect(m_resetCodeModelAction, &QAction::triggered,
            m_modelManager, &ModelManager::resetCodeModel);
    mqmljstools->addAction(cmd);

    // watch task progress
    connect(ProgressManager::instance(), SIGNAL(taskStarted(Core::Id)),
            this, SLOT(onTaskStarted(Core::Id)));
    connect(ProgressManager::instance(), SIGNAL(allTasksFinished(Core::Id)),
            this, SLOT(onAllTasksFinished(Core::Id)));

    return true;
}

void QmlJSToolsPlugin::extensionsInitialized()
{
    m_modelManager->delayedInitialization();
}

ExtensionSystem::IPlugin::ShutdownFlag QmlJSToolsPlugin::aboutToShutdown()
{
    return SynchronousShutdown;
}

void QmlJSToolsPlugin::onTaskStarted(Id type)
{
    if (type == QmlJS::Constants::TASK_INDEX)
        m_resetCodeModelAction->setEnabled(false);
}

void QmlJSToolsPlugin::onAllTasksFinished(Id type)
{
    if (type == QmlJS::Constants::TASK_INDEX)
        m_resetCodeModelAction->setEnabled(true);
}
