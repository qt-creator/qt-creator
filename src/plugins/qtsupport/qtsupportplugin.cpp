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

#include "qtsupportplugin.h"

#include "codegenerator.h"
#include "codegensettingspage.h"
#include "customexecutablerunconfiguration.h"
#include "desktopqtversionfactory.h"
#include "gettingstartedwelcomepage.h"
#include "qtkitinformation.h"
#include "qtoptionspage.h"
#include "qtversionmanager.h"
#include "uicodemodelsupport.h"
#include "winceqtversionfactory.h"

#include "profilereader.h"

#include <coreplugin/icore.h>
#include <coreplugin/jsexpander.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>
#include <projectexplorer/target.h>

#include <utils/macroexpander.h>
#include <utils/mimetypes/mimedatabase.h>

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

    Utils::MimeDatabase::addMimeTypes(QLatin1String(":qtsupport/QtSupport.mimetypes.xml"));

    JsExpander::registerQObjectForJs(QLatin1String("QtSupport"), new CodeGenerator);

    addAutoReleasedObject(new QtVersionManager);
    addAutoReleasedObject(new DesktopQtVersionFactory);
    addAutoReleasedObject(new WinCeQtVersionFactory);
    addAutoReleasedObject(new UiCodeModelManager);

    addAutoReleasedObject(new CodeGenSettingsPage);
    addAutoReleasedObject(new QtOptionsPage);

    ExamplesWelcomePage *welcomePage;
    welcomePage = new ExamplesWelcomePage;
    addAutoReleasedObject(welcomePage);
    welcomePage->setShowExamples(true);

    welcomePage = new ExamplesWelcomePage;
    addAutoReleasedObject(welcomePage);
    addAutoReleasedObject(new CustomExecutableRunConfigurationFactory);

    ProjectExplorer::KitManager::registerKitInformation(new QtKitInformation);

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
        tr("Full path to the target bin directory of the current project's Qt version."
           " You probably want %1 instead.").arg(QString::fromLatin1(kHostBins)),
        []() { return qmakeProperty("QT_INSTALL_BINS"); });
}

bool QtSupportPlugin::delayedInitialize()
{
    return QtVersionManager::delayedInitialize();
}
