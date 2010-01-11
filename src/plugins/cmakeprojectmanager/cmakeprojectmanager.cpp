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

#include "cmakeprojectmanager.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"

#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/environment.h>
#include <qtconcurrent/QtConcurrentTools>
#include <QtCore/QtConcurrentRun>
#include <QtCore/QSettings>
#include <QtGui/QFormLayout>
#include <QtGui/QBoxLayout>
#include <QtGui/QDesktopServices>
#include <QtGui/QApplication>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QSpacerItem>

using namespace CMakeProjectManager::Internal;

CMakeManager::CMakeManager(CMakeSettingsPage *cmakeSettingsPage)
    : m_settingsPage(cmakeSettingsPage)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_projectContext = uidm->uniqueIdentifier(CMakeProjectManager::Constants::PROJECTCONTEXT);
    m_projectLanguage = uidm->uniqueIdentifier(ProjectExplorer::Constants::LANG_CXX);    
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

bool CMakeManager::isCMakeExecutableValid() const
{
    return m_settingsPage->isCMakeExecutableValid();
}

void CMakeManager::setCMakeExecutable(const QString &executable)
{
    m_settingsPage->setCMakeExecutable(executable);
}

bool CMakeManager::hasCodeBlocksMsvcGenerator() const
{
    return m_settingsPage->hasCodeBlocksMsvcGenerator();
}

// TODO need to refactor this out
// we probably want the process instead of this function
// cmakeproject then could even run the cmake process in the background, adding the files afterwards
// sounds like a plan
void CMakeManager::createXmlFile(QProcess *proc, const QStringList &arguments, const QString &sourceDirectory, const QDir &buildDirectory, const ProjectExplorer::Environment &env, const QString &generator)
{
    // We create a cbp file, only if we didn't find a cbp file in the base directory
    // Yet that can still override cbp files in subdirectories
    // And we are creating tons of files in the source directories
    // All of that is not really nice.
    // The mid term plan is to move away from the CodeBlocks Generator and use our own
    // QtCreator generator, which actually can be very similar to the CodeBlock Generator

    // TODO we need to pass on the same paremeters as the cmakestep
    QString buildDirectoryPath = buildDirectory.absolutePath();
    buildDirectory.mkpath(buildDirectoryPath);
    proc->setWorkingDirectory(buildDirectoryPath);
    proc->setProcessChannelMode(QProcess::MergedChannels);
    proc->setEnvironment(env.toStringList());

    const QString srcdir = buildDirectory.exists(QLatin1String("CMakeCache.txt")) ? QString(QLatin1Char('.')) : sourceDirectory;    
    proc->start(cmakeExecutable(), QStringList() << srcdir << arguments << generator);
}

QString CMakeManager::findCbpFile(const QDir &directory)
{
    // Find the cbp file
    //   TODO the cbp file is named like the project() command in the CMakeList.txt file
    //   so this method below could find the wrong cbp file, if the user changes the project()
    //   2name
    foreach (const QString &cbpFile , directory.entryList()) {
        if (cbpFile.endsWith(QLatin1String(".cbp")))
            return directory.path() + QLatin1Char('/') + cbpFile;
    }
    return QString::null;
}

// This code is duplicated from qtversionmanager
QString CMakeManager::qtVersionForQMake(const QString &qmakePath)
{
    QProcess qmake;
    qmake.start(qmakePath, QStringList(QLatin1String("--version")));
    if (!qmake.waitForFinished())
        return false;
    QString output = qmake.readAllStandardOutput();
    QRegExp regexp(QLatin1String("(QMake version|Qmake version:)[\\s]*([\\d.]*)"));
    regexp.indexIn(output);
    if (regexp.cap(2).startsWith(QLatin1String("2."))) {
        QRegExp regexp2(QLatin1String("Using Qt version[\\s]*([\\d\\.]*)"));
        regexp2.indexIn(output);
        return regexp2.cap(1);
    }
    return QString();
}

/////
// CMakeSettingsPage
////


CMakeSettingsPage::CMakeSettingsPage()
    :  m_pathchooser(0), m_process(0)
{
    Core::ICore *core = Core::ICore::instance();
    QSettings * settings = core->settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));
    m_cmakeExecutable = settings->value(QLatin1String("cmakeExecutable")).toString();
    QFileInfo fi(m_cmakeExecutable);
    if (!fi.exists() || !fi.isExecutable())
        m_cmakeExecutable = findCmakeExecutable();
    fi.setFile(m_cmakeExecutable);
    if (fi.exists() && fi.isExecutable()) {
        // Run it to find out more
        m_state = RUNNING;
        startProcess();
    } else {
        m_state = INVALID;
    }

    settings->endGroup();
}

void CMakeSettingsPage::startProcess()
{
    m_process = new QProcess();

    connect(m_process, SIGNAL(finished(int)),
            this, SLOT(cmakeFinished()));

    m_process->start(m_cmakeExecutable, QStringList(QLatin1String("--help")));
    m_process->waitForStarted();
}

void CMakeSettingsPage::cmakeFinished()
{
    if (m_process) {
        QString response = m_process->readAll();
        QRegExp versionRegexp(QLatin1String("^cmake version ([\\d\\.]*)"));
        versionRegexp.indexIn(response);

        //m_supportsQtCreator = response.contains(QLatin1String("QtCreator"));
        m_hasCodeBlocksMsvcGenerator = response.contains(QLatin1String("CodeBlocks - NMake Makefiles"));
        m_version = versionRegexp.cap(1);
        if (!(versionRegexp.capturedTexts().size() > 3))
            m_version += QLatin1Char('.') + versionRegexp.cap(3);

        if (m_version.isEmpty())
            m_state = INVALID;
        else
            m_state = VALID;

        m_process->deleteLater();
        m_process = 0;
    }
}

bool CMakeSettingsPage::isCMakeExecutableValid()
{
    if (m_state == RUNNING) {
        disconnect(m_process, SIGNAL(finished(int)),
                   this, SLOT(cmakeFinished()));
        m_process->waitForFinished();
        // Parse the output now
        cmakeFinished();
    }
    return m_state == VALID;
}

CMakeSettingsPage::~CMakeSettingsPage()
{
    if (m_process)
        m_process->waitForFinished();
    delete m_process;
}

QString CMakeSettingsPage::findCmakeExecutable() const
{
    ProjectExplorer::Environment env = ProjectExplorer::Environment::systemEnvironment();
    return env.searchInPath(QLatin1String("cmake"));
}

QString CMakeSettingsPage::id() const
{
    return QLatin1String("CMake");
}

QString CMakeSettingsPage::displayName() const
{
    return tr("CMake");
}

QString CMakeSettingsPage::category() const
{
    return QLatin1String("M.CMake");
}

QString CMakeSettingsPage::displayCategory() const
{
    return tr("CMake");
}

QWidget *CMakeSettingsPage::createPage(QWidget *parent)
{
    QWidget *outerWidget = new QWidget(parent);
    QVBoxLayout *outerLayout = new QVBoxLayout(outerWidget);
    QGroupBox *groupBox = new QGroupBox;
    outerLayout->addWidget(groupBox);
    outerLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    QFormLayout *formLayout = new QFormLayout(groupBox);
    m_pathchooser = new Utils::PathChooser;
    m_pathchooser->setExpectedKind(Utils::PathChooser::Command);
    formLayout->addRow(tr("Executable:"), m_pathchooser);
    m_pathchooser->setPath(cmakeExecutable());
    return outerWidget;
}

void CMakeSettingsPage::updateInfo()
{
    QFileInfo fi(m_cmakeExecutable);
    if (fi.exists() && fi.isExecutable()) {
        // Run it to find out more
        m_state = RUNNING;
        startProcess();
    } else {
        m_state = INVALID;
    }
    saveSettings();
}

void CMakeSettingsPage::saveSettings() const
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));
    settings->setValue(QLatin1String("cmakeExecutable"), m_cmakeExecutable);
    settings->endGroup();
}

void CMakeSettingsPage::apply()
{
    if (m_cmakeExecutable == m_pathchooser->path())
        return;
    m_cmakeExecutable = m_pathchooser->path();
    updateInfo();
}

void CMakeSettingsPage::finish()
{

}

QString CMakeSettingsPage::cmakeExecutable() const
{
    return m_cmakeExecutable;
}

void CMakeSettingsPage::setCMakeExecutable(const QString &executable)
{
    if (m_cmakeExecutable == executable)
        return;
    m_cmakeExecutable = executable;
    updateInfo();
}

bool CMakeSettingsPage::hasCodeBlocksMsvcGenerator() const
{
    return m_hasCodeBlocksMsvcGenerator;
}
