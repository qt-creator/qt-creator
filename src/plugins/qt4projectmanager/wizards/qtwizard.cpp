/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qtwizard.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "modulespage.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <cpptools/cpptoolsconstants.h>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTextStream>
#include <QtCore/QSettings>
#include <QtCore/QVariant>

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
QtWizard::QtWizard(const QString &name, const QString &description, const QIcon &icon) :
    Core::BaseFileWizard(wizardParameters(name, description, icon))
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
    if (!ProjectExplorer::ProjectExplorerPlugin::instance()->openProject(proFileName)) {
        *errorMessage = tr("The project %1 could not be opened.").arg(proFileName);
        return false;
    }
    return true;
}

QString QtWizard::templateDir() const
{
    QString rc = Core::ICore::instance()->resourcePath();
    rc += QLatin1String("/templates/qt4project");
    return rc;
}

bool QtWizard::lowerCaseFiles()
{
    QString lowerCaseSettingsKey = QLatin1String(CppTools::Constants::CPPTOOLS_SETTINGSGROUP);
    lowerCaseSettingsKey += QLatin1Char('/');
    lowerCaseSettingsKey += QLatin1String(CppTools::Constants::LOWERCASE_CPPFILES_KEY);
    const bool lowerCaseDefault = CppTools::Constants::lowerCaseFilesDefault;
    return Core::ICore::instance()->settings()->value(lowerCaseSettingsKey, QVariant(lowerCaseDefault)).toBool();
}

bool QtWizard::showModulesPageForApplications()
{
    return false;
}

bool QtWizard::showModulesPageForLibraries()
{
    return true;
}

// ----------------- BaseQt4ProjectWizardDialog
BaseQt4ProjectWizardDialog::BaseQt4ProjectWizardDialog(bool showModulesPage, QWidget *parent) :
    ProjectExplorer::BaseProjectWizardDialog(parent),
    m_modulesPage(0)
{
    init(showModulesPage);
}

BaseQt4ProjectWizardDialog::BaseQt4ProjectWizardDialog(bool showModulesPage,
                                                       Utils::ProjectIntroPage *introPage,
                                                       int introId, QWidget *parent) :
    ProjectExplorer::BaseProjectWizardDialog(introPage, introId, parent),
    m_modulesPage(0)
{
    init(showModulesPage);
}

void BaseQt4ProjectWizardDialog::init(bool showModulesPage)
{
    if (showModulesPage)
        m_modulesPage = new ModulesPage;
}

void BaseQt4ProjectWizardDialog::addModulesPage(int id)
{
    if (m_modulesPage) {
        if (id >= 0) {
            setPage(id, m_modulesPage);
        } else {
            addPage(m_modulesPage);
        }
    }
}

QString BaseQt4ProjectWizardDialog::selectedModules() const
{
    return m_modulesPage ? m_modulesPage->selectedModules() : m_selectedModules;
}

void BaseQt4ProjectWizardDialog::setSelectedModules(const QString &modules, bool lock)
{   
    if (m_modulesPage) {
        foreach(const QString &module, modules.split(QLatin1Char(' '))) {
            m_modulesPage->setModuleSelected(module, true);
            m_modulesPage->setModuleEnabled(module, !lock);
        }
    } else {
        m_selectedModules = modules;
    }
}

QString BaseQt4ProjectWizardDialog::deselectedModules() const
{
    return m_modulesPage ? m_modulesPage->deselectedModules() : m_deselectedModules;
}

void BaseQt4ProjectWizardDialog::setDeselectedModules(const QString &modules)
{
    if (m_modulesPage) {
        foreach(const QString &module, modules.split(QLatin1Char(' ')))
            m_modulesPage->setModuleSelected(module, false);
    } else {
        m_deselectedModules = modules;
    }
}

