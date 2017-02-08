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

#include "qtsupportplugin.h"

#include "codegenerator.h"
#include "codegensettingspage.h"
#include "desktopqtversionfactory.h"
#include "gettingstartedwelcomepage.h"
#include "qtkitinformation.h"
#include "qtoptionspage.h"
#include "qtversionmanager.h"
#include "winceqtversionfactory.h"
#include "uicgenerator.h"
#include "qscxmlcgenerator.h"

#include "profilereader.h"

#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <utils/macroexpander.h>

#include <QtPlugin>

static const char kHostBins[] = "CurrentProject:QT_HOST_BINS";
static const char kInstallBins[] = "CurrentProject:QT_INSTALL_BINS";

using namespace Core;
using namespace QtSupport;
using namespace QtSupport::Internal;

bool QtSupportPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments);
    Q_UNUSED(errorMessage);
    QMakeParser::initialize();
    ProFileEvaluator::initialize();
    new ProFileCacheManager(this);

    JsExpander::registerQObjectForJs(QLatin1String("QtSupport"), new CodeGenerator);

    addAutoReleasedObject(new QtVersionManager);
    addAutoReleasedObject(new DesktopQtVersionFactory);
    addAutoReleasedObject(new WinCeQtVersionFactory);

    addAutoReleasedObject(new CodeGenSettingsPage);
    addAutoReleasedObject(new QtOptionsPage);

    addAutoReleasedObject(new ExamplesWelcomePage(true)); // Examples
    addAutoReleasedObject(new ExamplesWelcomePage(false)); // Tutorials

    ProjectExplorer::KitManager::registerKitInformation(new QtKitInformation);

    (void) new UicGeneratorFactory(this);
    (void) new QScxmlcGeneratorFactory(this);

    QtVersionManager::initialized();

    return true;
}

static QString qmakeProperty(const char *propertyName)
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectTree::currentProject();
    if (!project || !project->activeTarget())
        return QString();

    const BaseQtVersion *qtVersion = QtKitInformation::qtVersion(project->activeTarget()->kit());
    if (!qtVersion)
        return QString();
    return qtVersion->qmakeProperty(propertyName);
}

void QtSupportPlugin::extensionsInitialized()
{
    Utils::MacroExpander *expander = Utils::globalMacroExpander();

    expander->registerVariable(kHostBins,
        tr("Full path to the host bin directory of the current project's Qt version."),
        []() { return qmakeProperty("QT_HOST_BINS"); });

    expander->registerVariable(kInstallBins,
        tr("Full path to the target bin directory of the current project's Qt version.<br>"
           "You probably want %1 instead.").arg(QString::fromLatin1(kHostBins)),
        []() { return qmakeProperty("QT_INSTALL_BINS"); });
}
