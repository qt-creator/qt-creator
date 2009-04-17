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

#include "qt4runconfiguration.h"

#include "makestep.h"
#include "profilereader.h"
#include "qt4nodes.h"
#include "qt4project.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/variablemanager.h>
#include <coreplugin/ifile.h>
#include <projectexplorer/buildstep.h>
#include <utils/qtcassert.h>

#include <QtGui/QFormLayout>
#include <QtGui/QInputDialog>
#include <QtGui/QLabel>
#include <QtGui/QCheckBox>

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::ApplicationRunConfiguration;
using ProjectExplorer::PersistentSettingsReader;
using ProjectExplorer::PersistentSettingsWriter;

Qt4RunConfiguration::Qt4RunConfiguration(Qt4Project *pro, const QString &proFilePath)
    : ApplicationRunConfiguration(pro),
      m_proFilePath(proFilePath),
      m_runMode(Gui),
      m_userSetName(false),
      m_configWidget(0),
      m_executableLabel(0),
      m_workingDirectoryLabel(0),
      m_cachedTargetInformationValid(false),
      m_isUsingDyldImageSuffix(false)
{
    if (!m_proFilePath.isEmpty())
        setName(QFileInfo(m_proFilePath).completeBaseName());
    else
        setName(tr("Qt4RunConfiguration"));

    connect(pro, SIGNAL(activeBuildConfigurationChanged()),
            this, SLOT(invalidateCachedTargetInformation()));
}

Qt4RunConfiguration::~Qt4RunConfiguration()
{
}

QString Qt4RunConfiguration::type() const
{
    return "Qt4ProjectManager.Qt4RunConfiguration";
}


//////
/// Qt4RunConfigurationWidget
/////

Qt4RunConfigurationWidget::Qt4RunConfigurationWidget(Qt4RunConfiguration *qt4RunConfiguration, QWidget *parent)
    : QWidget(parent),
    m_qt4RunConfiguration(qt4RunConfiguration),
    m_ignoreChange(false),
    m_usingDyldImageSuffix(0),
    m_isShown(false)
{
    QFormLayout *toplayout = new QFormLayout(this);
    toplayout->setMargin(0);

    QLabel *nameLabel = new QLabel(tr("Name:"));
    m_nameLineEdit = new QLineEdit(m_qt4RunConfiguration->name());
    nameLabel->setBuddy(m_nameLineEdit);
    toplayout->addRow(nameLabel, m_nameLineEdit);

    m_executableLabel = new QLabel(m_qt4RunConfiguration->executable());
    toplayout->addRow(tr("Executable:"), m_executableLabel);

    m_workingDirectoryLabel = new QLabel(m_qt4RunConfiguration->workingDirectory());
    toplayout->addRow(tr("Working Directory:"), m_workingDirectoryLabel);

    QLabel *argumentsLabel = new QLabel(tr("&Arguments:"));
    m_argumentsLineEdit = new QLineEdit(ProjectExplorer::Environment::joinArgumentList(qt4RunConfiguration->commandLineArguments()));
    argumentsLabel->setBuddy(m_argumentsLineEdit);
    toplayout->addRow(argumentsLabel, m_argumentsLineEdit);

    m_useTerminalCheck = new QCheckBox(tr("Run in &Terminal"));
    m_useTerminalCheck->setChecked(m_qt4RunConfiguration->runMode() == ProjectExplorer::ApplicationRunConfiguration::Console);
    toplayout->addRow(QString(), m_useTerminalCheck);

#ifdef Q_OS_MAC
    m_usingDyldImageSuffix = new QCheckBox(tr("Use debug version of frameworks (DYLD_IMAGE_SUFFIX=_debug)"));
    m_usingDyldImageSuffix->setChecked(m_qt4RunConfiguration->isUsingDyldImageSuffix());
    toplayout->addRow(QString(), m_usingDyldImageSuffix);
    connect(m_usingDyldImageSuffix, SIGNAL(toggled(bool)),
            this, SLOT(usingDyldImageSuffixToggled(bool)));
#endif

    connect(m_argumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(setCommandLineArguments(QString)));
    connect(m_nameLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(nameEdited(QString)));
    connect(m_useTerminalCheck, SIGNAL(toggled(bool)),
            this, SLOT(termToggled(bool)));

    connect(qt4RunConfiguration, SIGNAL(commandLineArgumentsChanged(QString)),
            this, SLOT(commandLineArgumentsChanged(QString)));
    connect(qt4RunConfiguration, SIGNAL(nameChanged(QString)),
            this, SLOT(nameChanged(QString)));
    connect(qt4RunConfiguration, SIGNAL(runModeChanged(ProjectExplorer::ApplicationRunConfiguration::RunMode)),
            this, SLOT(runModeChanged(ProjectExplorer::ApplicationRunConfiguration::RunMode)));
    connect(qt4RunConfiguration, SIGNAL(usingDyldImageSuffixChanged(bool)),
            this, SLOT(usingDyldImageSuffixChanged(bool)));

    connect(qt4RunConfiguration, SIGNAL(effectiveTargetInformationChanged()),
            this, SLOT(effectiveTargetInformationChanged()), Qt::QueuedConnection);
}

void Qt4RunConfigurationWidget::setCommandLineArguments(const QString &args)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setCommandLineArguments(args);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::nameEdited(const QString &name)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->nameEdited(name);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::termToggled(bool on)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setRunMode(on ? ApplicationRunConfiguration::Console
                                         : ApplicationRunConfiguration::Gui);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::usingDyldImageSuffixToggled(bool state)
{
    m_ignoreChange = true;
    m_qt4RunConfiguration->setUsingDyldImageSuffix(state);
    m_ignoreChange = false;
}

void Qt4RunConfigurationWidget::commandLineArgumentsChanged(const QString &args)
{
    if (!m_ignoreChange)
        m_argumentsLineEdit->setText(args);
}

void Qt4RunConfigurationWidget::nameChanged(const QString &name)
{
    if (!m_ignoreChange)
        m_nameLineEdit->setText(name);
}

void Qt4RunConfigurationWidget::runModeChanged(ApplicationRunConfiguration::RunMode runMode)
{
    if (!m_ignoreChange)
        m_useTerminalCheck->setChecked(runMode == ApplicationRunConfiguration::Console);
}

void Qt4RunConfigurationWidget::usingDyldImageSuffixChanged(bool state)
{
    if (!m_ignoreChange && m_usingDyldImageSuffix)
        m_usingDyldImageSuffix->setChecked(state);
}

void Qt4RunConfigurationWidget::effectiveTargetInformationChanged()
{
    if (m_isShown) {
        m_executableLabel->setText(QDir::toNativeSeparators(m_qt4RunConfiguration->executable()));
        m_workingDirectoryLabel->setText(QDir::toNativeSeparators(m_qt4RunConfiguration->workingDirectory()));
    }
}

void Qt4RunConfigurationWidget::showEvent(QShowEvent *event)
{
    m_isShown = true;
    effectiveTargetInformationChanged();
    QWidget::showEvent(event);
}

void Qt4RunConfigurationWidget::hideEvent(QHideEvent *event)
{
    m_isShown = false;
    QWidget::hideEvent(event);
}

////// TODO c&p above
QWidget *Qt4RunConfiguration::configurationWidget()
{
    return new Qt4RunConfigurationWidget(this, 0);
}

void Qt4RunConfiguration::save(PersistentSettingsWriter &writer) const
{
    const QDir projectDir = QFileInfo(project()->file()->fileName()).absoluteDir();
    writer.saveValue("CommandLineArguments", m_commandLineArguments);
    writer.saveValue("ProFile", projectDir.relativeFilePath(m_proFilePath));
    writer.saveValue("UserSetName", m_userSetName);
    writer.saveValue("UseTerminal", m_runMode == Console);
    writer.saveValue("UseDyldImageSuffix", m_isUsingDyldImageSuffix);
    ApplicationRunConfiguration::save(writer);
}

void Qt4RunConfiguration::restore(const PersistentSettingsReader &reader)
{    
    ApplicationRunConfiguration::restore(reader);
    const QDir projectDir = QFileInfo(project()->file()->fileName()).absoluteDir();
    m_commandLineArguments = reader.restoreValue("CommandLineArguments").toStringList();
    m_proFilePath = projectDir.filePath(reader.restoreValue("ProFile").toString());
    m_userSetName = reader.restoreValue("UserSetName").toBool();
    m_runMode = reader.restoreValue("UseTerminal").toBool() ? Console : Gui;
    m_isUsingDyldImageSuffix = reader.restoreValue("UseDyldImageSuffix").toBool();
    if (!m_proFilePath.isEmpty()) {
        m_cachedTargetInformationValid = false;
        if (!m_userSetName)
            setName(QFileInfo(m_proFilePath).completeBaseName());
    }
}

QString Qt4RunConfiguration::executable() const
{
    const_cast<Qt4RunConfiguration *>(this)->updateTarget();
    return m_executable;
}

ApplicationRunConfiguration::RunMode Qt4RunConfiguration::runMode() const
{
    return m_runMode;
}

bool Qt4RunConfiguration::isUsingDyldImageSuffix() const
{
    return m_isUsingDyldImageSuffix;
}

void Qt4RunConfiguration::setUsingDyldImageSuffix(bool state)
{
    m_isUsingDyldImageSuffix = state;
    emit usingDyldImageSuffixChanged(state);
}

QString Qt4RunConfiguration::workingDirectory() const
{
    const_cast<Qt4RunConfiguration *>(this)->updateTarget();
    return m_workingDir;
}

QStringList Qt4RunConfiguration::commandLineArguments() const
{
    return m_commandLineArguments;
}

ProjectExplorer::Environment Qt4RunConfiguration::environment() const
{
    Qt4Project *pro = qobject_cast<Qt4Project *>(project());
    Q_ASSERT(pro);
    QString config = pro->activeBuildConfiguration();
    ProjectExplorer::Environment env = pro->environment(pro->activeBuildConfiguration());
    if (m_isUsingDyldImageSuffix) {
        env.set("DYLD_IMAGE_SUFFIX", "_debug");
    }
    return env;
}

void Qt4RunConfiguration::setCommandLineArguments(const QString &argumentsString)
{
    m_commandLineArguments = ProjectExplorer::Environment::parseCombinedArgString(argumentsString);
    emit commandLineArgumentsChanged(argumentsString);
}

void Qt4RunConfiguration::setRunMode(RunMode runMode)
{
    m_runMode = runMode;
    emit runModeChanged(runMode);
}

void Qt4RunConfiguration::nameEdited(const QString &name)
{
    if (name == "") {
        setName(tr("Qt4RunConfiguration"));
        m_userSetName = false;
    } else {
        setName(name);
        m_userSetName = true;
    }
    emit nameChanged(name);
}

QString Qt4RunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

void Qt4RunConfiguration::updateTarget()
{
    if (m_cachedTargetInformationValid)
        return;
    //qDebug()<<"updateTarget";
    Qt4Project *pro = static_cast<Qt4Project *>(project());
    ProFileReader *reader = pro->createProFileReader();
    reader->setCumulative(false);
    reader->setQtVersion(pro->qtVersion(pro->activeBuildConfiguration()));

    // Find out what flags we pass on to qmake, this code is duplicated in the qmake step
    QtVersion::QmakeBuildConfig defaultBuildConfiguration = pro->qtVersion(pro->activeBuildConfiguration())->defaultBuildConfig();
    QtVersion::QmakeBuildConfig projectBuildConfiguration = QtVersion::QmakeBuildConfig(pro->qmakeStep()->value(pro->activeBuildConfiguration(), "buildConfiguration").toInt());
    QStringList addedUserConfigArguments;
    QStringList removedUserConfigArguments;
    if ((defaultBuildConfiguration & QtVersion::BuildAll) && !(projectBuildConfiguration & QtVersion::BuildAll))
        removedUserConfigArguments << "debug_and_release";
    if (!(defaultBuildConfiguration & QtVersion::BuildAll) && (projectBuildConfiguration & QtVersion::BuildAll))
        addedUserConfigArguments << "debug_and_release";
    if ((defaultBuildConfiguration & QtVersion::DebugBuild) && !(projectBuildConfiguration & QtVersion::DebugBuild))
        addedUserConfigArguments << "release";
    if (!(defaultBuildConfiguration & QtVersion::DebugBuild) && (projectBuildConfiguration & QtVersion::DebugBuild))
        addedUserConfigArguments << "debug";

    reader->setUserConfigCmdArgs(addedUserConfigArguments, removedUserConfigArguments);

    QHash<QString, QStringList>::const_iterator it;

    if (!reader->readProFile(m_proFilePath)) {
        delete reader;
        Core::ICore::instance()->messageManager()->printToOutputPane(tr("Could not parse %1. The Qt4 run configuration %2 can not be started.").arg(m_proFilePath).arg(name()));
        return;
    }

    // Extract data
    QDir baseProjectDirectory = QFileInfo(project()->file()->fileName()).absoluteDir();
    QString relSubDir = baseProjectDirectory.relativeFilePath(QFileInfo(m_proFilePath).path());
    QDir baseBuildDirectory = project()->buildDirectory(project()->activeBuildConfiguration());
    QString baseDir = baseBuildDirectory.absoluteFilePath(relSubDir);

    //qDebug()<<relSubDir<<baseDir;

    // Working Directory
    if (reader->contains("DESTDIR")) {
        //qDebug()<<"reader contains destdir:"<<reader->value("DESTDIR");
        m_workingDir = reader->value("DESTDIR");
        if (QDir::isRelativePath(m_workingDir)) {
            m_workingDir = baseDir + QLatin1Char('/') + m_workingDir;
            //qDebug()<<"was relative and expanded to"<<m_workingDir;
        }
    } else {
        //qDebug()<<"reader didn't contain DESTDIR, setting to "<<baseDir;
        m_workingDir = baseDir;

#if defined(Q_OS_WIN)
        QString qmakeBuildConfig = "release";
        if (projectBuildConfiguration & QtVersion::DebugBuild)
            qmakeBuildConfig = "debug";
        if (!reader->contains("DESTDIR"))
            m_workingDir += QLatin1Char('/') + qmakeBuildConfig;
#endif
    }

#if defined (Q_OS_MAC)
    if (reader->values("CONFIG").contains("app_bundle")) {
        m_workingDir += QLatin1Char('/')
                   + reader->value("TARGET")
                   + QLatin1String(".app/Contents/MacOS");
    }
#endif

    m_workingDir = QDir::cleanPath(m_workingDir);
    m_executable = QDir::cleanPath(m_workingDir + QLatin1Char('/') + reader->value("TARGET"));
    //qDebug()<<"##### updateTarget sets:"<<m_workingDir<<m_executable;

#if defined (Q_OS_WIN)
    m_executable += QLatin1String(".exe");
#endif

    delete reader;

    m_cachedTargetInformationValid = true;

    emit effectiveTargetInformationChanged();
}

void Qt4RunConfiguration::invalidateCachedTargetInformation()
{
    m_cachedTargetInformationValid = false;
    emit effectiveTargetInformationChanged();
}

QString Qt4RunConfiguration::dumperLibrary() const
{
    Qt4Project *pro = qobject_cast<Qt4Project *>(project());
    QtVersion *version = pro->qtVersion(pro->activeBuildConfiguration());
    return version->dumperLibrary();
}


///
/// Qt4RunConfigurationFactory
/// This class is used to restore run settings (saved in .user files)
///

Qt4RunConfigurationFactory::Qt4RunConfigurationFactory()
{
}

Qt4RunConfigurationFactory::~Qt4RunConfigurationFactory()
{
}

// used to recreate the runConfigurations when restoring settings
bool Qt4RunConfigurationFactory::canCreate(const QString &type) const
{
    return type == "Qt4ProjectManager.Qt4RunConfiguration";
}

QSharedPointer<ProjectExplorer::RunConfiguration> Qt4RunConfigurationFactory::create
    (ProjectExplorer::Project *project, const QString &type)
{
    Qt4Project *p = qobject_cast<Qt4Project *>(project);
    Q_ASSERT(p);
    Q_ASSERT(type == "Qt4ProjectManager.Qt4RunConfiguration");
    // The right path is set in restoreSettings
    QSharedPointer<ProjectExplorer::RunConfiguration> rc(new Qt4RunConfiguration(p, QString::null));
    return rc;
}

QStringList Qt4RunConfigurationFactory::canCreate(ProjectExplorer::Project *pro) const
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(pro);
    if (qt4project)
        return QStringList();
    else
        return QStringList();
}

QString Qt4RunConfigurationFactory::nameForType(const QString &type) const
{
    Q_UNUSED(type);
    return "Run Qt4 application";
}

///
/// Qt4RunConfigurationFactoryUser
/// This class is used to create new RunConfiguration from the runsettings page
///

Qt4RunConfigurationFactoryUser::Qt4RunConfigurationFactoryUser()
{
}

Qt4RunConfigurationFactoryUser::~Qt4RunConfigurationFactoryUser()
{
}

bool Qt4RunConfigurationFactoryUser::canCreate(const QString &type) const
{
    Q_UNUSED(type);
    return false;
}

QSharedPointer<ProjectExplorer::RunConfiguration> Qt4RunConfigurationFactoryUser::create(ProjectExplorer::Project *project, const QString &type)
{
    Qt4Project *p = qobject_cast<Qt4Project *>(project);
    Q_ASSERT(p);

    QString fileName = type.mid(QString("Qt4RunConfiguration.").size());
    return QSharedPointer<ProjectExplorer::RunConfiguration>(new Qt4RunConfiguration(p, fileName));
}

QStringList Qt4RunConfigurationFactoryUser::canCreate(ProjectExplorer::Project *pro) const
{
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(pro);
    if (qt4project) {
        QStringList applicationProFiles;
        QList<Qt4ProFileNode *> list = qt4project->applicationProFiles();
        foreach (Qt4ProFileNode * node, list) {
            applicationProFiles.append("Qt4RunConfiguration." + node->path());
        }
        return applicationProFiles;
    } else {
        return QStringList();
    }
}

QString Qt4RunConfigurationFactoryUser::nameForType(const QString &type) const
{
    QString fileName = type.mid(QString("Qt4RunConfiguration.").size());
    return QFileInfo(fileName).completeBaseName();
}
