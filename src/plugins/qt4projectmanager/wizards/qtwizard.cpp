/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "qtwizard.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/projectexplorer.h>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

static inline Core::BaseFileWizardParameters
    wizardParameters(const QString &name,
                     const QString &description,
                     const QIcon &icon)
{
    Core::BaseFileWizardParameters rc(Core::IWizard::ProjectWizard);
    rc.setCategory(QLatin1String("Projects"));
    rc.setTrCategory("Projects");
    rc.setIcon(icon);
    rc.setName(name);
    rc.setDescription(description);
    return rc;
}

// -------------------- QtWizard
QtWizard::QtWizard(Core::ICore *core, const QString &name,
                   const QString &description, const QIcon &icon) :
    Core::BaseFileWizard(wizardParameters(name, description, icon), core),
    m_projectExplorer(ExtensionSystem::PluginManager::instance()->getObject<ProjectExplorer::ProjectExplorerPlugin>())
{
}

QString QtWizard::sourceSuffix()  const
{
    return preferredSuffix(QLatin1String(Constants::CPP_SOURCE_MIMETYPE));
}

QString QtWizard::headerSuffix()  const
{
    return preferredSuffix(QLatin1String(Constants::CPP_HEADER_MIMETYPE));
}

QString QtWizard::formSuffix()    const
{
    return preferredSuffix(QLatin1String(Constants::FORM_MIMETYPE));
}

QString QtWizard::profileSuffix() const
{
    return preferredSuffix(QLatin1String(Constants::PROFILE_MIMETYPE));
}

bool QtWizard::postGenerateFiles(const Core::GeneratedFiles &l, QString *errorMessage)
{
    // Post-Generate: Open the project
    const QString proFileName = l.back().path();
    if (!m_projectExplorer->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

QString QtWizard::templateDir() const
{
    QString rc = core()->resourcePath();
    rc += QLatin1String("/templates/qt4project");
    return rc;
}
