/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "cmakeprojectmanager.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/uniqueidmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/environment.h>
#include <QtCore/QSettings>
#include <QFormLayout>

using namespace CMakeProjectManager::Internal;

CMakeManager::CMakeManager(CMakeSettingsPage *cmakeSettingsPage)
    : m_settingsPage(cmakeSettingsPage)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_projectContext = uidm->uniqueIdentifier(CMakeProjectManager::Constants::PROJECTCONTEXT);
    m_projectLanguage = uidm->uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);
}

CMakeSettingsPage::~CMakeSettingsPage()
{

}

int CMakeManager::projectContext() const
{
    return m_projectContext;
}

int CMakeManager::projectLanguage() const
{
    return m_projectLanguage;
}

ProjectExplorer::Project *CMakeManager::openProject(const QString &fileName)
{
    // TODO check wheter this project is already opened
    // Check that we have a cmake executable first
    // Look at the settings first
    QString cmakeExecutable = m_settingsPage->cmakeExecutable();
    if (cmakeExecutable.isNull())
        m_settingsPage->askUserForCMakeExecutable();
    cmakeExecutable = m_settingsPage->cmakeExecutable();
    if (cmakeExecutable.isNull())
        return 0;
    return new CMakeProject(this, fileName);
}

QString CMakeManager::mimeType() const
{
    return Constants::CMAKEMIMETYPE;
}

/////
// CMakeSettingsPage
////

CMakeSettingsPage::CMakeSettingsPage()
{
    Core::ICore *core = Core::ICore::instance();
    QSettings * settings = core->settings();
    settings->beginGroup("CMakeSettings");
    m_cmakeExecutable =  settings->value("cmakeExecutable").toString();
    settings->endGroup();
    updateCachedInformation();
}

void CMakeSettingsPage::updateCachedInformation() const
{
    // We find out two things:
    // Does this cmake version support a QtCreator generator
    // and the version
    QFileInfo fi(m_cmakeExecutable);
    if (!fi.exists()) {
        m_version.clear();
        m_supportsQtCreator = false;
    }
    QProcess cmake;
    cmake.start(m_cmakeExecutable, QStringList()<<"--help");
    cmake.waitForFinished();
    QString response = cmake.readAll();
    QRegExp versionRegexp("^cmake version ([*\\d\\.]*)-(|patch (\\d*))(|\\r)\\n");
    versionRegexp.indexIn(response);

    m_supportsQtCreator = response.contains("QtCreator");
    m_version = versionRegexp.cap(1);
    if (!versionRegexp.capturedTexts().size()>3)
        m_version += "." + versionRegexp.cap(3);
}

QString CMakeSettingsPage::findCmakeExecutable() const
{
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    return env.searchInPath("cmake");
}


QString CMakeSettingsPage::name() const
{
    return "CMake";
}

QString CMakeSettingsPage::category() const
{
    return "CMake";
}

QString CMakeSettingsPage::trCategory() const
{
    return tr("CMake");
}

QWidget *CMakeSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    QFormLayout *fl = new QFormLayout(w);
    m_pathchooser = new Core::Utils::PathChooser(w);
    m_pathchooser->setExpectedKind(Core::Utils::PathChooser::Command);
    fl->addRow("CMake executable", m_pathchooser);
    m_pathchooser->setPath(cmakeExecutable());
    return w;
}

void CMakeSettingsPage::saveSettings() const
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup("CMakeSettings");
    settings->setValue("cmakeExecutable", m_cmakeExecutable);
    settings->endGroup();
}

void CMakeSettingsPage::apply()
{
    m_cmakeExecutable = m_pathchooser->path();
    updateCachedInformation();
    saveSettings();
}

void CMakeSettingsPage::finish()
{

}

QString CMakeSettingsPage::cmakeExecutable() const
{
    if (m_cmakeExecutable.isEmpty()) {
        m_cmakeExecutable = findCmakeExecutable();
        if (!m_cmakeExecutable.isEmpty()) {
            updateCachedInformation();
            saveSettings();
        }
    }
    return m_cmakeExecutable;
}

void CMakeSettingsPage::askUserForCMakeExecutable()
{
    // TODO implement
    // That is ideally add a label to the settings page, which says something
    // to the effect: please configure the cmake executable
    // and show the settings page
    // ensure that we rehide the label in the finish() function
    // But to test that i need an environment without cmake, e.g. windows
}
