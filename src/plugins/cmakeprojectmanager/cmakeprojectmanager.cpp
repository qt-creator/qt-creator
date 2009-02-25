/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "cmakeprojectmanager.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/environment.h>
#include <qtconcurrent/QtConcurrentTools>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QSettings>
#include <QtGui/QFormLayout>

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

QString CMakeManager::cmakeExecutable() const
{
    return m_settingsPage->cmakeExecutable();
}

// TODO need to refactor this out
// we probably want the process instead of this function
// cmakeproject then could even run the cmake process in the background, adding the files afterwards
// sounds like a plan
QString CMakeManager::createXmlFile(const QStringList &arguments, const QString &sourceDirectory, const QDir &buildDirectory)
{
    // We create a cbp file, only if we didn't find a cbp file in the base directory
    // Yet that can still override cbp files in subdirectories
    // And we are creating tons of files in the source directories
    // All of that is not really nice.
    // The mid term plan is to move away from the CodeBlocks Generator and use our own
    // QtCreator generator, which actually can be very similar to the CodeBlock Generator


    // TODO we need to pass on the same paremeters as the cmakestep
    QString buildDirectoryPath = buildDirectory.absolutePath();
    qDebug()<<"Creating cbp file in"<<buildDirectoryPath;
    buildDirectory.mkpath(buildDirectoryPath);
    QProcess cmake;
    cmake.setWorkingDirectory(buildDirectoryPath);
    cmake.start(cmakeExecutable(), QStringList() << sourceDirectory << arguments << "-GCodeBlocks - Unix Makefiles");
    qDebug()<<cmakeExecutable()<<sourceDirectory << arguments;
    cmake.waitForFinished(-1);
    cmake.setProcessChannelMode(QProcess::MergedChannels);
    QString output = cmake.readAll();
    qDebug()<<"cmake output: \n"<<output;
    return output;
}


/////
// CMakeRunner
////
// TODO give a better name, what this class is to update cached information
// about a cmake executable, with qtconcurrent
// The nifty feature of this class is that it does so in a seperate thread,
// not blocking the main thread

CMakeRunner::CMakeRunner()
    : m_cacheUpToDate(false)
{

}

void CMakeRunner::run(QFutureInterface<void> &fi)
{
    m_mutex.lock();
    QString executable = m_executable;
    m_mutex.unlock();
    QProcess cmake;
    cmake.start(executable, QStringList()<<"--help");
    cmake.waitForFinished();
    QString response = cmake.readAll();
    QRegExp versionRegexp("^cmake version ([*\\d\\.]*)-(|patch (\\d*))(|\\r)\\n");
    versionRegexp.indexIn(response);

    m_mutex.lock();
    m_supportsQtCreator = response.contains("QtCreator");
    m_version = versionRegexp.cap(1);
    if (!versionRegexp.capturedTexts().size()>3)
        m_version += "." + versionRegexp.cap(3);
    m_cacheUpToDate = true;
    m_mutex.unlock();
    fi.reportFinished();
}

void CMakeRunner::setExecutable(const QString &executable)
{
    waitForUpToDate();
    m_mutex.lock();
    m_executable = executable;
    m_cacheUpToDate = false;
    m_mutex.unlock();
    m_future = QtConcurrent::run(&CMakeRunner::run, this);
}

QString CMakeRunner::executable() const
{
    waitForUpToDate();
    m_mutex.lock();
    QString result = m_executable;
    m_mutex.unlock();
    return result;
}

QString CMakeRunner::version() const
{
    waitForUpToDate();
    m_mutex.lock();
    QString result = m_version;
    m_mutex.unlock();
    return result;
}

bool CMakeRunner::supportsQtCreator() const
{
    waitForUpToDate();
    m_mutex.lock();
    bool result = m_supportsQtCreator;
    m_mutex.unlock();
    return result;
}

void CMakeRunner::waitForUpToDate() const
{
    m_future.waitForFinished();
}

/////
// CMakeSettingsPage
////


CMakeSettingsPage::CMakeSettingsPage()
{
    Core::ICore *core = Core::ICore::instance();
    QSettings * settings = core->settings();
    settings->beginGroup("CMakeSettings");
    m_cmakeRunner.setExecutable(settings->value("cmakeExecutable").toString());
    settings->endGroup();
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
    settings->setValue("cmakeExecutable", m_cmakeRunner.executable());
    settings->endGroup();
}

void CMakeSettingsPage::apply()
{
    m_cmakeRunner.setExecutable(m_pathchooser->path());
    saveSettings();
}

void CMakeSettingsPage::finish()
{

}

QString CMakeSettingsPage::cmakeExecutable() const
{
    if (m_cmakeRunner.executable().isEmpty()) {
        QString cmakeExecutable = findCmakeExecutable();
        if (!cmakeExecutable.isEmpty()) {
            m_cmakeRunner.setExecutable(cmakeExecutable);
            saveSettings();
        }
    }
    return m_cmakeRunner.executable();
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
