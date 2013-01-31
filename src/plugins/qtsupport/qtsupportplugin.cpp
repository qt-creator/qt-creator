/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qtsupportplugin.h"

#include "customexecutablerunconfiguration.h"
#include "qtoptionspage.h"
#include "qtkitinformation.h"
#include "qtversionmanager.h"

#include "profilereader.h"

#include "gettingstartedwelcomepage.h"

#include <coreplugin/variablemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>

#include <QtPlugin>
#include <QMenu>

static const char kHostBins[] = "CurrentProject:QT_HOST_BINS";
static const char kInstallBins[] = "CurrentProject:QT_INSTALL_BINS";

using namespace QtSupport;
using namespace QtSupport::Internal;

bool QtSupportPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);
    QMakeParser::initialize();
    ProFileEvaluator::initialize();
    new ProFileCacheManager(this);

    QtVersionManager *mgr = new QtVersionManager;
    addAutoReleasedObject(mgr);

    QtFeatureProvider *featureMgr = new QtFeatureProvider;
    addAutoReleasedObject(featureMgr);

    addAutoReleasedObject(new QtOptionsPage);

    ExamplesWelcomePage *welcomePage;
    welcomePage = new ExamplesWelcomePage;
    addAutoReleasedObject(welcomePage);

    welcomePage = new ExamplesWelcomePage;
    welcomePage->setShowExamples(true);
    addAutoReleasedObject(welcomePage);

    GettingStartedWelcomePage *gettingStartedWelcomePage = new GettingStartedWelcomePage;
    addAutoReleasedObject(gettingStartedWelcomePage);

    addAutoReleasedObject(new CustomExecutableRunConfigurationFactory);

    return true;
}

void QtSupportPlugin::extensionsInitialized()
{
    Core::VariableManager *vm = Core::VariableManager::instance();
    vm->registerVariable(kHostBins,
        tr("Full path to the host bin directory of the current project's Qt version."));
    vm->registerVariable(kInstallBins,
        tr("Full path to the target bin directory of the current project's Qt version."
           " You probably want %1 instead.").arg(QString::fromLatin1(kHostBins)));
    connect(vm, SIGNAL(variableUpdateRequested(QByteArray)),
            this, SLOT(updateVariable(QByteArray)));

    QtVersionManager::instance()->extensionsInitialized();
    ProjectExplorer::KitManager::instance()->registerKitInformation(new QtKitInformation);
}

bool QtSupportPlugin::delayedInitialize()
{
    return QtVersionManager::instance()->delayedInitialize();
}

void QtSupportPlugin::updateVariable(const QByteArray &variable)
{
    if (variable != kHostBins && variable != kInstallBins)
        return;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::currentProject();
    if (!project || !project->activeTarget()) {
        Core::VariableManager::instance()->remove(variable);
        return;
    }

    const BaseQtVersion *qtVersion = QtKitInformation::qtVersion(project->activeTarget()->kit());
    if (!qtVersion) {
        Core::VariableManager::instance()->remove(variable);
        return;
    }

    QString value = qtVersion->qmakeProperty(variable == kHostBins ? "QT_HOST_BINS" : "QT_INSTALL_BINS");
    Core::VariableManager::instance()->insert(variable, value);
}

Q_EXPORT_PLUGIN(QtSupportPlugin)
