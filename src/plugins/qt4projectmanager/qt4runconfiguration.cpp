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

#include "qt4runconfiguration.h"

#include "makestep.h"
#include "profilereader.h"
#include "qt4nodes.h"
#include "qt4project.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/variablemanager.h>
#include <projectexplorer/buildstep.h>
#include <utils/qtcassert.h>

#include <QtGui/QFormLayout>
#include <QtGui/QInputDialog>

using namespace Qt4ProjectManager::Internal;
using namespace Qt4ProjectManager;
using ProjectExplorer::ApplicationRunConfiguration;
using ProjectExplorer::PersistentSettingsReader;
using ProjectExplorer::PersistentSettingsWriter;

Qt4RunConfiguration::Qt4RunConfiguration(Qt4Project *pro, QString proFilePath)
    : ApplicationRunConfiguration(pro), m_proFilePath(proFilePath), m_userSetName(false)
{
    setName(tr("Qt4RunConfiguration"));
    if (!m_proFilePath.isEmpty()) {
        updateCachedValues();
        setName(QFileInfo(m_proFilePath).baseName());
    }
}

Qt4RunConfiguration::~Qt4RunConfiguration()
{
}

QString Qt4RunConfiguration::type() const
{
    return "Qt4ProjectManager.Qt4RunConfiguration";
}

QWidget *Qt4RunConfiguration::configurationWidget()
{
    QWidget *configWidget = new QWidget;
    QFormLayout *toplayout = new QFormLayout(configWidget);
    toplayout->setMargin(0);

    QLabel *nameLabel = new QLabel(tr("Name:"));
    QLineEdit *nameLineEdit = new QLineEdit(name());
    nameLabel->setBuddy(nameLineEdit);
    toplayout->addRow(nameLabel, nameLineEdit);

    QLabel *executableLabel = new QLabel(tr("Executable:"));
    QLabel *executableLabel2 = new QLabel(executable());
    toplayout->addRow(executableLabel, executableLabel2);

    QLabel *workingDirectoryLabel = new QLabel(tr("Working Directory:"));
    QLabel *workingDirectoryLabel2 = new QLabel(workingDirectory());
    toplayout->addRow(workingDirectoryLabel, workingDirectoryLabel2);

    QLabel *argumentsLabel = new QLabel(tr("&Arguments:"));
    QLineEdit *argumentsLineEdit = new QLineEdit(ProjectExplorer::Environment::joinArgumentList(commandLineArguments()));
    argumentsLabel->setBuddy(argumentsLineEdit);
    toplayout->addRow(argumentsLabel, argumentsLineEdit);

    connect(argumentsLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(setCommandLineArguments(const QString&)));

    connect(nameLineEdit, SIGNAL(textEdited(const QString&)),
            this, SLOT(nameEdited(const QString&)));

    return configWidget;
}

void Qt4RunConfiguration::save(PersistentSettingsWriter &writer) const
{
    writer.saveValue("CommandLineArguments", m_commandLineArguments);
    writer.saveValue("ProFile", m_proFilePath);
    writer.saveValue("UserSetName", m_userSetName);
    ApplicationRunConfiguration::save(writer);
}

void Qt4RunConfiguration::restore(const PersistentSettingsReader &reader)
{
    ApplicationRunConfiguration::restore(reader);
    m_commandLineArguments = reader.restoreValue("CommandLineArguments").toStringList();
    m_proFilePath = reader.restoreValue("ProFile").toString();
    m_userSetName = reader.restoreValue("UserSetName").toBool();
    if (!m_proFilePath.isEmpty()) {
        updateCachedValues();
        if (!m_userSetName)
            setName(QFileInfo(m_proFilePath).baseName());
    }
}

QString Qt4RunConfiguration::executable() const
{
    return resolveVariables(project()->activeBuildConfiguration(), m_executable);
}

ApplicationRunConfiguration::RunMode Qt4RunConfiguration::runMode() const
{
    return m_runMode;
}

QString Qt4RunConfiguration::workingDirectory() const
{
    return resolveVariables(project()->activeBuildConfiguration(), m_workingDir);
}

QStringList Qt4RunConfiguration::commandLineArguments() const
{
    return m_commandLineArguments;
}

ProjectExplorer::Environment Qt4RunConfiguration::environment() const
{
    Qt4Project *pro = qobject_cast<Qt4Project *>(project());
    Q_ASSERT(pro);
    return pro->environment(pro->activeBuildConfiguration());
}

void Qt4RunConfiguration::setCommandLineArguments(const QString &argumentsString)
{
    m_commandLineArguments = ProjectExplorer::Environment::parseCombinedArgString(argumentsString);
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
}

QString Qt4RunConfiguration::proFilePath() const
{
    return m_proFilePath;
}

// and needs to be reloaded.
// Check wheter it is
void Qt4RunConfiguration::updateCachedValues()
{
    ProFileReader *reader = static_cast<Qt4Project *>(project())->createProFileReader();
    if (!reader->readProFile(m_proFilePath)) {
        delete reader;
        Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
        core->messageManager()->printToOutputPane(QString("Could not parse %1. The Qt4 run configuration %2 can not be started.").arg(m_proFilePath).arg(name()));
        return;
    }

    QString destDir;

    if (reader->contains("DESTDIR")) {
        // TODO Can return different destdirs for different scopes!
        destDir = reader->value("DESTDIR");
        if (QDir::isRelativePath(destDir)) {
            destDir = "${BASEDIR}" + QLatin1Char('/') + destDir;
        }
    } else {
        destDir = "${BASEDIR}";
#if defined(Q_OS_WIN)
        if (!reader->contains("DESTDIR"))
            destDir += QLatin1Char('/') + "${QMAKE_BUILDCONFIG}";
#endif
    }

#if defined (Q_OS_MAC)
    if (!reader->values("-CONFIG").contains("app_bundle")) {
        destDir += QLatin1Char('/')
                   + "${QMAKE_TARGET}"
                   + QLatin1String(".app/Contents/MacOS");
    }
#endif
    m_workingDir = destDir;
    m_executable = destDir + QLatin1Char('/') + "${QMAKE_TARGET}";

#if defined (Q_OS_WIN)
    m_executable += QLatin1String(".exe");
#endif

    m_targets = reader->values(QLatin1String("TARGET"));

    m_srcDir = QFileInfo(m_proFilePath).path();
    const QStringList config = reader->values(QLatin1String("CONFIG"));
    m_runMode = ProjectExplorer::ApplicationRunConfiguration::Gui;

    delete reader;
}

QString Qt4RunConfiguration::resolveVariables(const QString &buildConfiguration, const QString& in) const
{
    detectQtShadowBuild(buildConfiguration);
    
    QString relSubDir = QFileInfo(project()->file()->fileName()).absoluteDir().relativeFilePath(m_srcDir);
    QString baseDir = QDir(project()->buildDirectory(buildConfiguration)).absoluteFilePath(relSubDir);

    Core::VariableManager *vm = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>()->variableManager();
    if (!vm)
        return QString();
    QString dest;
    bool found = false;
    vm->insert("QMAKE_BUILDCONFIG", qmakeBuildConfigFromBuildConfiguration(buildConfiguration));
    vm->insert("BASEDIR", baseDir);


    /*
      TODO This is a hack to detect correct target (there might be different targets in
      different scopes)
    */

    // This code also works for workingDirectory,
    // since directories are executable.
    foreach (const QString &target, m_targets) {
        dest = in;
        vm->insert("QMAKE_TARGET", target);
        dest = QDir::cleanPath(vm->resolve(dest));
        vm->remove("QMAKE_TARGET");
        QFileInfo fi(dest);
        if (fi.exists() && (fi.isExecutable() || dest.endsWith(".js"))) {
            found = true;
            break;
        }
    }
    vm->remove("BASEDIR");
    vm->remove("QMAKE_BUILDCONFIG");
    if (found)
        return dest;
    else
        return QString();
}

/* This function tries to find out wheter qmake/make will put the binary in "/debug/" or in "/release/"
   That is this function is strictly only for windows.
   We look wheter make gets an explicit parameter "debug" or "release"
   That works because if we have either debug or release there then it is surely a
   debug_and_release buildconfiguration and thus we are put in a subdirectory.

   Now if there is no explicit debug or release parameter, then we need to look at what qmake's CONFIG
   value is, if it is not debug_and_release, we don't care and return ""
   otherwise we look at wheter the default is debug or not

   Note: When fixing this function consider those cases
       qmake CONFIG+=debug_and_release CONFIG+=debug
       make release
    => we should return release

        qmake CONFIG+=debug_and_release CONFIG+=debug
        make
    => we should return debug

        qmake CONFIG-=debug_and_release CONFIG+=debug
        make
    => we should return "", since the executable is not put in a subdirectory

   Not a function to be proud of
*/
QString Qt4RunConfiguration::qmakeBuildConfigFromBuildConfiguration(const QString &buildConfigurationName) const
{
    MakeStep *ms = qobject_cast<Qt4Project *>(project())->makeStep();
    QStringList makeargs = ms->value(buildConfigurationName, "makeargs").toStringList();
    if (makeargs.contains("debug"))
        return "debug";
    else if (makeargs.contains("release"))
        return "release";

    // Oh we don't have an explicit make argument
    QMakeStep *qs = qobject_cast<Qt4Project *>(project())->qmakeStep();
    QVariant qmakeBuildConfiguration = qs->value(buildConfigurationName, "buildConfiguration");
    if (qmakeBuildConfiguration.isValid()) {
        QtVersion::QmakeBuildConfig projectBuildConfiguration = QtVersion::QmakeBuildConfig(qmakeBuildConfiguration.toInt());
        if (projectBuildConfiguration & QtVersion::DebugBuild)
            return "debug";
        else
            return "release";
    } else {
        // Old sytle always CONFIG+=debug_and_release
        if (qobject_cast<Qt4Project *>(project())->qtVersion(buildConfigurationName)->defaultBuildConfig() & QtVersion::DebugBuild)
            return "debug";
        else
            return "release";
    }

    // enable us to infer the right string
    return "";
}

/*!
  Handle special case were a subproject of the qt directory is opened, and
  qt was configured to be built as a shadow build -> also build in the sub-
  project in the correct shadow build directory.
  */
void Qt4RunConfiguration::detectQtShadowBuild(const QString &buildConfiguration) const
{
    if (project()->activeBuildConfiguration() == buildConfiguration)
        return;

    const QString currentQtDir = static_cast<Qt4Project *>(project())->qtDir(buildConfiguration);
    const QString qtSourceDir = static_cast<Qt4Project *>(project())->qtVersion(buildConfiguration)->sourcePath();

    // if the project is a sub-project of Qt and Qt was shadow-built then automatically
    // adjust the build directory of the sub-project.
    if (project()->file()->fileName().startsWith(qtSourceDir) && qtSourceDir != currentQtDir) {
        project()->setValue(buildConfiguration, "useShadowBuild", true);
        QString buildDir = QFileInfo(project()->file()->fileName()).absolutePath();
        buildDir.replace(qtSourceDir, currentQtDir);
        project()->setValue(buildConfiguration, "buildDirectory", buildDir);
        project()->setValue(buildConfiguration, "autoShadowBuild", true);
    }
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
    return QFileInfo(fileName).baseName();
}
