/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cmakeopenprojectwizard.h"
#include "cmakeprojectmanager.h"
#include "cmakeprojectconstants.h"
#include "cmakeproject.h"

#include <utils/synchronousprocess.h>
#include <utils/qtcprocess.h>

#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <utils/QtConcurrentTools>
#include <QtConcurrentRun>
#include <QCoreApplication>
#include <QSettings>
#include <QDateTime>
#include <QFormLayout>
#include <QBoxLayout>
#include <QDesktopServices>
#include <QApplication>
#include <QLabel>
#include <QGroupBox>
#include <QSpacerItem>

using namespace CMakeProjectManager::Internal;

CMakeManager::CMakeManager(CMakeSettingsPage *cmakeSettingsPage)
    : m_settingsPage(cmakeSettingsPage)
{
    ProjectExplorer::ProjectExplorerPlugin *projectExplorer = ProjectExplorer::ProjectExplorerPlugin::instance();
    connect(projectExplorer, SIGNAL(aboutToShowContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)),
            this, SLOT(updateContextMenu(ProjectExplorer::Project*,ProjectExplorer::Node*)));

    Core::ActionContainer *mbuild =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_BUILDPROJECT);
    Core::ActionContainer *mproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_PROJECTCONTEXT);
    Core::ActionContainer *msubproject =
            Core::ActionManager::actionContainer(ProjectExplorer::Constants::M_SUBPROJECTCONTEXT);

    const Core::Context projectContext(CMakeProjectManager::Constants::PROJECTCONTEXT);

    m_runCMakeAction = new QAction(QIcon(), tr("Run CMake"), this);
    Core::Command *command = Core::ActionManager::registerAction(m_runCMakeAction,
                                                                 Constants::RUNCMAKE, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mbuild->addAction(command, ProjectExplorer::Constants::G_BUILD_DEPLOY);
    connect(m_runCMakeAction, SIGNAL(triggered()), this, SLOT(runCMake()));

    m_runCMakeActionContextMenu = new QAction(QIcon(), tr("Run CMake"), this);
    command = Core::ActionManager::registerAction(m_runCMakeActionContextMenu,
                                                  Constants::RUNCMAKECONTEXTMENU, projectContext);
    command->setAttribute(Core::Command::CA_Hide);
    mproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    msubproject->addAction(command, ProjectExplorer::Constants::G_PROJECT_BUILD);
    connect(m_runCMakeActionContextMenu, SIGNAL(triggered()), this, SLOT(runCMakeContextMenu()));

}

void CMakeManager::updateContextMenu(ProjectExplorer::Project *project, ProjectExplorer::Node *node)
{
    Q_UNUSED(node);
    m_contextProject = project;
}

void CMakeManager::runCMake()
{
    runCMake(ProjectExplorer::ProjectExplorerPlugin::currentProject());
}

void CMakeManager::runCMakeContextMenu()
{
    runCMake(m_contextProject);
}

void CMakeManager::runCMake(ProjectExplorer::Project *project)
{
    if (!project)
        return;
    CMakeProject *cmakeProject = qobject_cast<CMakeProject *>(project);
    if (!cmakeProject || !cmakeProject->activeTarget() || !cmakeProject->activeTarget()->activeBuildConfiguration())
        return;

    CMakeBuildConfiguration *bc
            = qobject_cast<CMakeBuildConfiguration *>(cmakeProject->activeTarget()->activeBuildConfiguration());
    if (!bc)
        return;

    CMakeOpenProjectWizard copw(this,
                                cmakeProject->projectDirectory(),
                                bc->buildDirectory(),
                                CMakeOpenProjectWizard::WantToUpdate,
                                bc->environment());
    if (copw.exec() == QDialog::Accepted)
        cmakeProject->triggerBuildSystemEvaluation();
}

ProjectExplorer::Project *CMakeManager::openProject(const QString &fileName, QString *errorString)
{
    Q_UNUSED(errorString)
    // TODO check whether this project is already opened
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

// need to refactor this out
// we probably want the process instead of this function
// cmakeproject then could even run the cmake process in the background, adding the files afterwards
// sounds like a plan
void CMakeManager::createXmlFile(Utils::QtcProcess *proc, const QString &arguments,
                                 const QString &sourceDirectory, const QDir &buildDirectory,
                                 const Utils::Environment &env, const QString &generator)
{
    // We create a cbp file, only if we didn't find a cbp file in the base directory
    // Yet that can still override cbp files in subdirectories
    // And we are creating tons of files in the source directories
    // All of that is not really nice.
    // The mid term plan is to move away from the CodeBlocks Generator and use our own
    // QtCreator generator, which actually can be very similar to the CodeBlock Generator
    QString buildDirectoryPath = buildDirectory.absolutePath();
    buildDirectory.mkpath(buildDirectoryPath);
    proc->setWorkingDirectory(buildDirectoryPath);
    proc->setEnvironment(env);

    const QString srcdir = buildDirectory.exists(QLatin1String("CMakeCache.txt")) ?
                QString(QLatin1Char('.')) : sourceDirectory;
    QString args;
    Utils::QtcProcess::addArg(&args, srcdir);
    Utils::QtcProcess::addArgs(&args, arguments);
    Utils::QtcProcess::addArg(&args, generator);
    proc->setCommand(cmakeExecutable(), args);
    proc->start();
}

QString CMakeManager::findCbpFile(const QDir &directory)
{
    // Find the cbp file
    //   the cbp file is named like the project() command in the CMakeList.txt file
    //   so this method below could find the wrong cbp file, if the user changes the project()
    //   2name
    QDateTime t;
    QString file;
    foreach (const QString &cbpFile , directory.entryList()) {
        if (cbpFile.endsWith(QLatin1String(".cbp"))) {
            QFileInfo fi(directory.path() + QLatin1Char('/') + cbpFile);
            if (t.isNull() || fi.lastModified() > t) {
                file = directory.path() + QLatin1Char('/') + cbpFile;
                t = fi.lastModified();
            }
        }
    }
    return file;
}

// This code is duplicated from qtversionmanager
QString CMakeManager::qtVersionForQMake(const QString &qmakePath)
{
    QProcess qmake;
    qmake.start(qmakePath, QStringList(QLatin1String("--version")));
    if (!qmake.waitForStarted()) {
        qWarning("Cannot start '%s': %s", qPrintable(qmakePath), qPrintable(qmake.errorString()));
        return QString();
    }
    if (!qmake.waitForFinished())      {
        Utils::SynchronousProcess::stopProcess(qmake);
        qWarning("Timeout running '%s'.", qPrintable(qmakePath));
        return QString();
    }
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
    :  m_pathchooser(0)
{
    setId(QLatin1String("Z.CMake"));
    setDisplayName(tr("CMake"));
    setCategory(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("ProjectExplorer",
       ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_TR_CATEGORY));
    setCategoryIcon(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY_ICON));

    m_userCmake.process = 0;
    m_pathCmake.process = 0;
    m_userCmake.hasCodeBlocksMsvcGenerator = false;
    m_pathCmake.hasCodeBlocksMsvcGenerator = false;
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));
    m_userCmake.executable = settings->value(QLatin1String("cmakeExecutable")).toString();
    settings->endGroup();

    updateInfo(&m_userCmake);
    m_pathCmake.executable = findCmakeExecutable();
    updateInfo(&m_pathCmake);
}

void CMakeSettingsPage::startProcess(CMakeValidator *cmakeValidator)
{
    cmakeValidator->process = new QProcess();

    if (cmakeValidator == &m_pathCmake) // ugly
        connect(cmakeValidator->process, SIGNAL(finished(int)),
                this, SLOT(userCmakeFinished()));
    else
        connect(cmakeValidator->process, SIGNAL(finished(int)),
                this, SLOT(pathCmakeFinished()));

    cmakeValidator->process->start(cmakeValidator->executable, QStringList(QLatin1String("--help")));
    cmakeValidator->process->waitForStarted();
}

void CMakeSettingsPage::userCmakeFinished()
{
    cmakeFinished(&m_userCmake);
}

void CMakeSettingsPage::pathCmakeFinished()
{
    cmakeFinished(&m_pathCmake);
}

void CMakeSettingsPage::cmakeFinished(CMakeValidator *cmakeValidator) const
{
    if (cmakeValidator->process) {
        cmakeValidator->process->waitForFinished();
        QString response = cmakeValidator->process->readAll();
        QRegExp versionRegexp(QLatin1String("^cmake version ([\\d\\.]*)"));
        versionRegexp.indexIn(response);

        //m_supportsQtCreator = response.contains(QLatin1String("QtCreator"));
        cmakeValidator->hasCodeBlocksMsvcGenerator = response.contains(QLatin1String("CodeBlocks - NMake Makefiles"));
        cmakeValidator->version = versionRegexp.cap(1);
        if (!(versionRegexp.capturedTexts().size() > 3))
            cmakeValidator->version += QLatin1Char('.') + versionRegexp.cap(3);

        if (cmakeValidator->version.isEmpty())
            cmakeValidator->state = CMakeValidator::INVALID;
        else
            cmakeValidator->state = CMakeValidator::VALID;

        cmakeValidator->process->deleteLater();
        cmakeValidator->process = 0;
    }
}

bool CMakeSettingsPage::isCMakeExecutableValid() const
{
    if (m_userCmake.state == CMakeValidator::RUNNING) {
        disconnect(m_userCmake.process, SIGNAL(finished(int)),
                   this, SLOT(userCmakeFinished()));
        m_userCmake.process->waitForFinished();
        // Parse the output now
        cmakeFinished(&m_userCmake);
    }

    if (m_userCmake.state == CMakeValidator::VALID)
        return true;
    if (m_pathCmake.state == CMakeValidator::RUNNING) {
        disconnect(m_userCmake.process, SIGNAL(finished(int)),
                   this, SLOT(pathCmakeFinished()));
        m_pathCmake.process->waitForFinished();
        // Parse the output now
        cmakeFinished(&m_pathCmake);
    }
    return m_pathCmake.state == CMakeValidator::VALID;
}

CMakeSettingsPage::~CMakeSettingsPage()
{
    if (m_userCmake.process)
        m_userCmake.process->waitForFinished();
    delete m_userCmake.process;
    if (m_pathCmake.process)
        m_pathCmake.process->waitForFinished();
    delete m_pathCmake.process;
}

QString CMakeSettingsPage::findCmakeExecutable() const
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    return env.searchInPath(QLatin1String("cmake"));
}

QWidget *CMakeSettingsPage::createPage(QWidget *parent)
{
    QWidget *outerWidget = new QWidget(parent);
    QFormLayout *formLayout = new QFormLayout(outerWidget);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_pathchooser = new Utils::PathChooser;
    m_pathchooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    formLayout->addRow(tr("Executable:"), m_pathchooser);
    formLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::Ignored, QSizePolicy::MinimumExpanding));
    m_pathchooser->setPath(m_userCmake.executable);
    return outerWidget;
}

void CMakeSettingsPage::updateInfo(CMakeValidator *cmakeValidator)
{
    QFileInfo fi(cmakeValidator->executable);
    if (fi.exists() && fi.isExecutable()) {
        // Run it to find out more
        cmakeValidator->state = CMakeValidator::RUNNING;
        startProcess(cmakeValidator);
    } else {
        cmakeValidator->state = CMakeValidator::INVALID;
    }
    saveSettings();
}

void CMakeSettingsPage::saveSettings() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("CMakeSettings"));
    settings->setValue(QLatin1String("cmakeExecutable"), m_userCmake.executable);
    settings->endGroup();
}

void CMakeSettingsPage::apply()
{
    if (!m_pathchooser) // page was never shown
        return;
    if (m_userCmake.executable == m_pathchooser->path())
        return;
    m_userCmake.executable = m_pathchooser->path();
    updateInfo(&m_userCmake);
}

void CMakeSettingsPage::finish()
{

}

QString CMakeSettingsPage::cmakeExecutable() const
{
    if (!isCMakeExecutableValid())
        return QString();
    if (m_userCmake.state == CMakeValidator::VALID)
        return m_userCmake.executable;
    else
        return m_pathCmake.executable;
}

void CMakeSettingsPage::setCMakeExecutable(const QString &executable)
{
    if (m_userCmake.executable == executable)
        return;
    m_userCmake.executable = executable;
    updateInfo(&m_userCmake);
}

bool CMakeSettingsPage::hasCodeBlocksMsvcGenerator() const
{
    if (!isCMakeExecutableValid())
        return false;
    if (m_userCmake.state == CMakeValidator::VALID)
        return m_userCmake.hasCodeBlocksMsvcGenerator;
    else
        return m_pathCmake.hasCodeBlocksMsvcGenerator;
}
