/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "userfileaccessor.h"

#include "buildconfiguration.h"
#include "persistentsettings.h"
#include "project.h"
#include "target.h"
#include "toolchain.h"

#include <coreplugin/ifile.h>
#include <utils/qtcassert.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

using namespace ProjectExplorer;

namespace {
const char * const USER_FILE_VERSION("ProjectExplorer.Project.Updater.FileVersion");
const char * const WAS_UPDATED("ProjectExplorer.Project.Updater.DidUpdate");
const char * const PROJECT_FILE_POSTFIX(".user");

// Version 0 is used in Qt Creator 1.3.x and
// (in a slighly differnt flavour) post 1.3 master.
class Version0Handler : public UserFileVersionHandler
{
public:
    Version0Handler();
    ~Version0Handler();

    int userFileVersion() const
    {
        return 0;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("1.3");
    }

    QVariantMap update(Project *project, const QVariantMap &map);

private:
    QVariantMap convertBuildConfigurations(Project *project, const QVariantMap &map);
    QVariantMap convertRunConfigurations(Project *project, const QVariantMap &map);
    QVariantMap convertBuildSteps(Project *project, const QVariantMap &map);
};

// Version 1 is used in master post Qt Creator 1.3.x.
// It was never used in any official release but is required for the
// transition to later versions (which introduce support for targets).
class Version1Handler : public UserFileVersionHandler
{
public:
    Version1Handler();
    ~Version1Handler();

    int userFileVersion() const
    {
        return 1;
    }

    QString displayUserFileVersion() const
    {
        return QLatin1String("1.3+git");
    }

    QVariantMap update(Project *project, const QVariantMap &map);

private:
    struct TargetDescription
    {
        TargetDescription(QString tid, QString dn) :
            id(tid),
            displayName(dn)
        {
        }

        TargetDescription(const TargetDescription &td) :
            id(td.id),
            displayName(td.displayName)
        {
        }

        QString id;
        QString displayName;
    };
};

//
// Helper functions:
//

QString fileNameFor(const QString &name) {
    QString baseName(name);
    QString environmentExtension(QString::fromLocal8Bit(qgetenv("QTC_EXTENSION")));
    if (!environmentExtension.isEmpty()) {
        // replace fishy characters:
        environmentExtension = environmentExtension.replace(QRegExp("[^a-zA-Z0-9_.-]"), QChar('_'));

        if (!environmentExtension.startsWith('.'))
            environmentExtension = environmentExtension.prepend(QLatin1Char('.'));
        return baseName + environmentExtension;
    }
    return baseName + QLatin1String(PROJECT_FILE_POSTFIX);
}

} // namespace

// -------------------------------------------------------------------------
// UserFileVersionHandler
// -------------------------------------------------------------------------

UserFileVersionHandler::UserFileVersionHandler()
{
}

UserFileVersionHandler::~UserFileVersionHandler()
{
}

// -------------------------------------------------------------------------
// UserFileAccessor
// -------------------------------------------------------------------------

UserFileAccessor::UserFileAccessor() :
    m_firstVersion(-1),
    m_lastVersion(-1)
{
    addVersionHandler(new Version0Handler);
    addVersionHandler(new Version1Handler);
}

UserFileAccessor::~UserFileAccessor()
{
    qDeleteAll(m_handlers);
}

QVariantMap UserFileAccessor::restoreSettings(Project *project)
{
    if (m_lastVersion < 0 || !project)
        return QVariantMap();

    QString fileName(fileNameFor(project->file()->fileName()));
    if (!QFile::exists(fileName))
        return QVariantMap();

    PersistentSettingsReader reader;
    reader.load(fileName);

    QVariantMap map(reader.restoreValues());

    // Get and verify file version:
    const int fileVersion(map.value(QLatin1String(USER_FILE_VERSION), 0).toInt());
    if (fileVersion < m_firstVersion || fileVersion > m_lastVersion + 1) {
        qWarning() << "File version" << fileVersion << "is not supported.";
        return QVariantMap();
    }

    // Do we need to do a update?
    if (fileVersion != m_lastVersion + 1) {
        map.insert(QLatin1String(WAS_UPDATED), true);
        QFile::copy(fileName, fileName + '.' + m_handlers.value(fileVersion)->displayUserFileVersion());
    }

    // Update:
    for (int i = fileVersion; i <= m_lastVersion; ++i)
        map = m_handlers.value(i)->update(project, map);

    map.insert(QLatin1String(USER_FILE_VERSION), m_lastVersion + 1);

    return map;
}

bool UserFileAccessor::saveSettings(Project *project, const QVariantMap &map)
{
    if (!project || map.isEmpty())
        return false;

    PersistentSettingsWriter writer;

    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i)
        writer.saveValue(i.key(), i.value());

    writer.saveValue(QLatin1String(USER_FILE_VERSION), m_lastVersion + 1);

    return writer.save(fileNameFor(project->file()->fileName()), "QtCreatorProject");
}

void UserFileAccessor::addVersionHandler(UserFileVersionHandler *handler)
{
    const int version(handler->userFileVersion());
    QTC_ASSERT(handler, return);
    QTC_ASSERT(version >= 0, return);
    QTC_ASSERT(!m_handlers.contains(version), return);
    QTC_ASSERT(m_handlers.isEmpty() ||
               (version == m_lastVersion + 1 || version == m_firstVersion - 1), return);

    if (m_handlers.isEmpty()) {
        m_firstVersion = version;
        m_lastVersion = version;
    } else {
        if (version < m_firstVersion)
            m_firstVersion = version;
        if (version > m_lastVersion)
            m_lastVersion = version;
    }

    m_handlers.insert(version, handler);

    // Postconditions:
    Q_ASSERT(m_lastVersion >= 0);
    Q_ASSERT(m_firstVersion >= 0);
    Q_ASSERT(m_lastVersion >= m_firstVersion);
    Q_ASSERT(m_handlers.count() == m_lastVersion - m_firstVersion + 1);
    for (int i = m_firstVersion; i < m_lastVersion; ++i)
        Q_ASSERT(m_handlers.contains(i));
}


// -------------------------------------------------------------------------
// Version0Handler
// -------------------------------------------------------------------------

Version0Handler::Version0Handler()
{
}

Version0Handler::~Version0Handler()
{
}

QVariantMap Version0Handler::convertBuildConfigurations(Project *project, const QVariantMap &map)
{
    Q_ASSERT(project);
    QVariantMap result;

    // Find a valid Id to use:
    QString id;
    if (project->id() == QLatin1String("GenericProjectManager.GenericProject"))
        id = QLatin1String("GenericProjectManager.GenericBuildConfiguration");
    else if (project->id() == QLatin1String("CMakeProjectManager.CMakeProject"))
        id = QLatin1String("CMakeProjectManager.CMakeBuildConfiguration");
    else if (project->id() == QLatin1String("Qt4ProjectManager.Qt4Project"))
        id = QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration");
    else
        return QVariantMap(); // QmlProjects do not(/no longer) have BuildConfigurations,
                              // or we do not know how to handle this.
    result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), id);

    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName")) {
            result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"),
                         i.value());
            continue;
        }

        if (id == QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration") ||
            id.startsWith(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration."))) {
            // Qt4BuildConfiguration:
            if (i.key() == QLatin1String("QtVersionId")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.QtVersionId"),
                              i.value().toInt());
            } else if (i.key() == QLatin1String("ToolChain")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.ToolChain"),
                              i.value());
            } else if (i.key() == QLatin1String("buildConfiguration")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.BuildConfiguration"),
                              i.value());
            } else if (i.key() == QLatin1String("userEnvironmentChanges")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.UserEnvironmentChanges"),
                              i.value());
            } else if (i.key() == QLatin1String("useShadowBuild")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.UseShadowBuild"),
                              i.value());
            } else if (i.key() == QLatin1String("clearSystemEnvironment")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.ClearSystemEnvironment"),
                              i.value());
            } else if (i.key() == QLatin1String("buildDirectory")) {
                result.insert(QLatin1String("Qt4ProjectManager.Qt4BuildConfiguration.BuildDirectory"),
                              i.value());
            } else {
                qWarning() << "Unknown Qt4BuildConfiguration Key found:" << i.key() << i.value();
            }
            continue;
        } else if (id == QLatin1String("CMakeProjectManager.CMakeBuildConfiguration")) {
            if (i.key() == QLatin1String("userEnvironmentChanges")) {
                result.insert(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.UserEnvironmentChanges"),
                              i.value());
            } else if (i.key() == QLatin1String("msvcVersion")) {
                result.insert(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.MsvcVersion"),
                              i.value());
            } else if (i.key() == QLatin1String("buildDirectory")) {
                result.insert(QLatin1String("CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory"),
                              i.value());
            } else {
                qWarning() << "Unknown CMakeBuildConfiguration Key found:" << i.key() << i.value();
            }
            continue;
        } else if (id == QLatin1String("GenericProjectManager.GenericBuildConfiguration")) {
            if (i.key() == QLatin1String("buildDirectory")) {
                result.insert(QLatin1String("GenericProjectManager.GenericBuildConfiguration.BuildDirectory"),
                              i.value());
            } else {
                qWarning() << "Unknown GenericBuildConfiguration Key found:" << i.key() << i.value();
            }
            continue;
        }
        qWarning() << "Unknown BuildConfiguration Key found:" << i.key() << i.value();
        qWarning() << "BuildConfiguration Id is:" << id;
    }

    return result;
}

QVariantMap Version0Handler::convertRunConfigurations(Project *project, const QVariantMap &map)
{
    Q_UNUSED(project);
    QVariantMap result;
    QString id;

    // Convert Id:
    id = map.value(QLatin1String("Id")).toString();
    if (id.isEmpty())
        id = map.value(QLatin1String("type")).toString();
    if (id.isEmpty())
        return QVariantMap();

    if (QLatin1String("Qt4ProjectManager.DeviceRunConfiguration") == id)
        id = QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration");
    if (QLatin1String("Qt4ProjectManager.EmulatorRunConfiguration") == id)
        id = QLatin1String("Qt4ProjectManager.S60EmulatorRunConfiguration");
    // no need to change the CMakeRunConfiguration, CustomExecutableRunConfiguration,
    //                       MaemoRunConfiguration or Qt4RunConfiguration

    result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), id);

    // Convert everything else:
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("Id") || i.key() == QLatin1String("type"))
            continue;
        if (i.key() == QLatin1String("RunConfiguration.name")) {
            result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"),
                         i.value());
        } else if (QLatin1String("CMakeProjectManager.CMakeRunConfiguration") == id) {
            if (i.key() == QLatin1String("CMakeRunConfiguration.Target"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.Target"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.WorkingDirectory"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.WorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.UserWorkingDirectory"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.UserWorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.UseTerminal"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.UseTerminal"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguation.Title"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguation.Title"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.Arguments"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.Arguments"), i.value());
            else if (i.key() == QLatin1String("CMakeRunConfiguration.UserEnvironmentChanges"))
                result.insert(QLatin1String("CMakeProjectManager.CMakeRunConfiguration.UserEnvironmentChanges"), i.value());
            else if (i.key() == QLatin1String("BaseEnvironmentBase"))
                result.insert(QLatin1String("CMakeProjectManager.BaseEnvironmentBase"), i.value());
            else
                qWarning() << "Unknown CMakeRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.ProFile"), i.value());
            else if (i.key() == QLatin1String("SigningMode"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SigningMode"), i.value());
            else if (i.key() == QLatin1String("CustomSignaturePath"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomSignaturePath"), i.value());
            else if (i.key() == QLatin1String("CustomKeyPath"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CustomKeyPath"), i.value());
            else if (i.key() == QLatin1String("SerialPortName"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.SerialPortName"), i.value());
            else if (i.key() == QLatin1String("CommunicationType"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CommunicationType"), i.value());
            else if (i.key() == QLatin1String("CommandLineArguments"))
                result.insert(QLatin1String("Qt4ProjectManager.S60DeviceRunConfiguration.CommandLineArguments"), i.value());
            else
                qWarning() << "Unknown S60DeviceRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.S60EmulatorRunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.S60EmulatorRunConfiguration.ProFile"), i.value());
            else
                qWarning() << "Unknown S60EmulatorRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.Qt4RunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.ProFile"), i.value());
            else if (i.key() == QLatin1String("CommandLineArguments"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.CommandLineArguments"), i.value());
            else if (i.key() == QLatin1String("UserSetName"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserSetName"), i.value());
            else if (i.key() == QLatin1String("UseTerminal"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UseTerminal"), i.value());
            else if (i.key() == QLatin1String("UseDyldImageSuffix"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UseDyldImageSuffix"), i.value());
            else if (i.key() == QLatin1String("UserEnvironmentChanges"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserEnvironmentChanges"), i.value());
            else if (i.key() == QLatin1String("BaseEnvironmentBase"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.BaseEnvironmentBase"), i.value());
            else if (i.key() == QLatin1String("UserSetWorkingDirectory"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserSetWorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("UserWorkingDirectory"))
                result.insert(QLatin1String("Qt4ProjectManager.Qt4RunConfiguration.UserWorkingDirectory"), i.value());
            else
                qWarning() << "Unknown Qt4RunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.MaemoRunConfiguration") == id) {
            if (i.key() == QLatin1String("ProFile"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.ProFile"), i.value());
            else if (i.key() == QLatin1String("Arguments"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.Arguments"), i.value());
            else if (i.key() == QLatin1String("Simulator"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.Simulator"), i.value());
            else if (i.key() == QLatin1String("DeviceId"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.DeviceId"), i.value());
            else if (i.key() == QLatin1String("LastDeployed"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.LastDeployed"), i.value());
            else if (i.key() == QLatin1String("DebuggingHelpersLastDeployed"))
                result.insert(QLatin1String("Qt4ProjectManager.MaemoRunConfiguration.DebuggingHelpersLastDeployed"), i.value());
            else
                qWarning() << "Unknown MaemoRunConfiguration key found:" << i.key() << i.value();
        } else if (QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration") == id) {
            if (i.key() == QLatin1String("Executable"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.Executable"), i.value());
            else if (i.key() == QLatin1String("Arguments"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.Arguments"), i.value());
            else if (i.key() == QLatin1String("WorkingDirectory"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.WorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("UseTerminal"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UseTerminal"), i.value());
            else if (i.key() == QLatin1String("UserSetName"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UserSetName"), i.value());
            else if (i.key() == QLatin1String("UserName"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UserName"), i.value());
            else if (i.key() == QLatin1String("UserEnvironmentChanges"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.UserEnvironmentChanges"), i.value());
            else if (i.key() == QLatin1String("BaseEnvironmentBase"))
                result.insert(QLatin1String("ProjectExplorer.CustomExecutableRunConfiguration.BaseEnvironmentBase"), i.value());
            else
                qWarning() << "Unknown CustomExecutableRunConfiguration key found:" << i.key() << i.value();
        } else {
            result.insert(i.key(), i.value());
        }
    }

    return result;
}

QVariantMap Version0Handler::convertBuildSteps(Project *project, const QVariantMap &map)
{
    Q_UNUSED(project);
    QVariantMap result;

    QString id(map.value(QLatin1String("Id")).toString());
    if (QLatin1String("GenericProjectManager.MakeStep") == id)
        id = QLatin1String("GenericProjectManager.GenericMakeStep");
    if (QLatin1String("projectexplorer.processstep") == id)
        id = QLatin1String("ProjectExplorer.ProcessStep");
    if (QLatin1String("trolltech.qt4projectmanager.make") == id)
        id = QLatin1String("Qt4ProjectManager.MakeStep");
    if (QLatin1String("trolltech.qt4projectmanager.qmake") == id)
        id = QLatin1String("QtProjectManager.QMakeBuildStep");
    // No need to change the CMake MakeStep.
    result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), id);

    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("Id"))
            continue;
        if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName")) {
            // skip this: Not needed.
            continue;
        }

        if (QLatin1String("GenericProjectManager.GenericMakeStep") == id) {
            if (i.key() == QLatin1String("buildTargets"))
                result.insert(QLatin1String("GenericProjectManager.GenericMakeStep.BuildTargets"), i.value());
            else if (i.key() == QLatin1String("makeArguments"))
                result.insert(QLatin1String("GenericProjectManager.GenericMakeStep.MakeArguments"), i.value());
            else if (i.key() == QLatin1String("makeCommand"))
                result.insert(QLatin1String("GenericProjectManager.GenericMakeStep.MakeCommand"), i.value());
            else
                qWarning() << "Unknown GenericMakeStep value found:" << i.key() << i.value();
            continue;
        } else if (QLatin1String("ProjectExplorer.ProcessStep") == id) {
            if (i.key() == QLatin1String("ProjectExplorer.ProcessStep.DisplayName"))
                result.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.DisplayName"), i.value());
            else if (i.key() == QLatin1String("abstractProcess.command"))
                result.insert(QLatin1String("ProjectExplorer.ProcessStep.Command"), i.value());
            else if ((i.key() == QLatin1String("abstractProcess.workingDirectory") ||
                      i.key() == QLatin1String("workingDirectory")) &&
                     !i.value().toString().isEmpty())
                    result.insert(QLatin1String("ProjectExplorer.ProcessStep.WorkingDirectory"), i.value());
            else if (i.key() == QLatin1String("abstractProcess.arguments"))
                result.insert(QLatin1String("ProjectExplorer.ProcessStep.Arguments"), i.value());
            else if (i.key() == QLatin1String("abstractProcess.enabled"))
                result.insert(QLatin1String("ProjectExplorer.ProcessStep.Enabled"), i.value());
            else
                qWarning() << "Unknown ProcessStep value found:" << i.key() << i.value();
        } else if (QLatin1String("Qt4ProjectManager.MakeStep") == id) {
            if (i.key() == QLatin1String("makeargs"))
                result.insert(QLatin1String("Qt4ProjectManager.MakeStep.MakeArguments"), i.value());
            else if (i.key() == QLatin1String("makeCmd"))
                result.insert(QLatin1String("Qt4ProjectManager.MakeStep.MakeCommand"), i.value());
            else if (i.key() == QLatin1String("clean"))
                result.insert(QLatin1String("Qt4ProjectManager.MakeStep.Clean"), i.value());
            else
                qWarning() << "Unknown Qt4MakeStep value found:" << i.key() << i.value();
        } else if (QLatin1String("QtProjectManager.QMakeBuildStep") == id) {
            if (i.key() == QLatin1String("qmakeArgs"))
                result.insert(QLatin1String("QtProjectManager.QMakeBuildStep.QMakeArguments"), i.value());
            else
                qWarning() << "Unknown Qt4QMakeStep value found:" << i.key() << i.value();
        } else if (QLatin1String("CMakeProjectManager.MakeStep") == id) {
            if (i.key() == QLatin1String("buildTargets"))
                result.insert(QLatin1String("CMakeProjectManager.MakeStep.BuildTargets"), i.value());
            else if (i.key() == QLatin1String("additionalArguments"))
                result.insert(QLatin1String("CMakeProjectManager.MakeStep.AdditionalArguments"), i.value());
            else if (i.key() == QLatin1String("clean"))
                result.insert(QLatin1String("CMakeProjectManager.MakeStep.Clean"), i.value());
            else
                qWarning() << "Unknown CMakeMakeStep value found:" << i.key() << i.value();
        } else {
            result.insert(i.key(), i.value());
        }
    }
    return result;
}

QVariantMap Version0Handler::update(Project *project, const QVariantMap &map)
{
    QVariantMap result;

    // "project": section is unused, just ignore it.

    // "buildconfigurations" and "buildConfiguration-":
    QStringList bcs(map.value(QLatin1String("buildconfigurations")).toStringList());
    QString active(map.value(QLatin1String("activebuildconfiguration")).toString());

    int count(0);
    foreach (const QString &bc, bcs) {
        // convert buildconfiguration:
        QString oldBcKey(QString::fromLatin1("buildConfiguration-") + bc);
        if (bc == active)
            result.insert(QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration"), count);

        QVariantMap tmp(map.value(oldBcKey).toMap());
        QVariantMap bcMap(convertBuildConfigurations(project, tmp));
        if (bcMap.isEmpty())
            continue;

        // buildsteps
        QStringList buildSteps(map.value(oldBcKey + QLatin1String("-buildsteps")).toStringList());

        if (buildSteps.isEmpty())
            // try lowercase version, too:-(
            buildSteps = map.value(QString::fromLatin1("buildconfiguration-") + bc + QLatin1String("-buildsteps")).toStringList();
        if (buildSteps.isEmpty())
            buildSteps = map.value(QLatin1String("buildsteps")).toStringList();

        int pos(0);
        foreach (const QString &bs, buildSteps) {
            // Watch out: Capitalization differs from oldBcKey!
            const QString localKey(QLatin1String("buildconfiguration-") + bc + QString::fromLatin1("-buildstep") + QString::number(pos));
            const QString globalKey(QString::fromLatin1("buildstep") + QString::number(pos));

            QVariantMap local(map.value(localKey).toMap());
            QVariantMap global(map.value(globalKey).toMap());

            for (QVariantMap::const_iterator i = global.constBegin(); i != global.constEnd(); ++i) {
                if (!local.contains(i.key()))
                    local.insert(i.key(), i.value());
                if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName") &&
                    local.value(i.key()).toString().isEmpty())
                    local.insert(i.key(), i.value());
            }
            local.insert(QLatin1String("Id"), bs);

            bcMap.insert(QString::fromLatin1("ProjectExplorer.BuildConfiguration.BuildStep.") + QString::number(pos),
                         convertBuildSteps(project, local));
            ++pos;
        }
        bcMap.insert(QLatin1String("ProjectExplorer.BuildConfiguration.BuildStepsCount"), pos);

        // cleansteps
        QStringList cleanSteps(map.value(oldBcKey + QLatin1String("-cleansteps")).toStringList());
        if (cleanSteps.isEmpty())
            // try lowercase version, too:-(
            cleanSteps = map.value(QString::fromLatin1("buildconfiguration-") + bc + QLatin1String("-cleansteps")).toStringList();
        if (cleanSteps.isEmpty())
            cleanSteps = map.value(QLatin1String("cleansteps")).toStringList();

        pos = 0;
        foreach (const QString &bs, cleanSteps) {
            // Watch out: Capitalization differs from oldBcKey!
            const QString localKey(QLatin1String("buildconfiguration-") + bc + QString::fromLatin1("-cleanstep") + QString::number(pos));
            const QString globalKey(QString::fromLatin1("cleanstep") + QString::number(pos));

            QVariantMap local(map.value(localKey).toMap());
            QVariantMap global(map.value(globalKey).toMap());

            for (QVariantMap::const_iterator i = global.constBegin(); i != global.constEnd(); ++i) {
                if (!local.contains(i.key()))
                    local.insert(i.key(), i.value());
                if (i.key() == QLatin1String("ProjectExplorer.BuildConfiguration.DisplayName") &&
                    local.value(i.key()).toString().isEmpty())
                    local.insert(i.key(), i.value());
            }
            local.insert(QLatin1String("Id"), bs);
            bcMap.insert(QString::fromLatin1("ProjectExplorer.BuildConfiguration.CleanStep.") + QString::number(pos),
                         convertBuildSteps(project, local));
            ++pos;
        }
        bcMap.insert(QLatin1String("ProjectExplorer.BuildConfiguration.CleanStepsCount"), pos);

        // Merge into result set:
        result.insert(QString::fromLatin1("ProjectExplorer.Project.BuildConfiguration.") + QString::number(count), bcMap);
        ++count;
    }
    result.insert(QLatin1String("ProjectExplorer.Project.BuildConfigurationCount"), count);

    // "RunConfiguration*":
    active = map.value(QLatin1String("activeRunConfiguration")).toString();
    count = 0;
    forever {
        QString prefix(QLatin1String("RunConfiguration") + QString::number(count) + '-');
        QVariantMap rcMap;
        for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
            if (!i.key().startsWith(prefix))
                continue;
            QString newKey(i.key().mid(prefix.size()));
            rcMap.insert(newKey, i.value());
        }
        if (rcMap.isEmpty()) {
            result.insert(QLatin1String("ProjectExplorer.Project.RunConfigurationCount"), count);
            break;
        }

        result.insert(QString::fromLatin1("ProjectExplorer.Project.RunConfiguration.") + QString::number(count),
                      convertRunConfigurations(project, rcMap));
        ++count;
    }

    // "defaultFileEncoding" (EditorSettings):
    QVariant codecVariant(map.value(QLatin1String("defaultFileEncoding")));
    if (codecVariant.isValid()) {
        QByteArray codec(codecVariant.toByteArray());
        QVariantMap editorSettingsMap;
        editorSettingsMap.insert(QLatin1String("EditorConfiguration.Codec"), codec);
        result.insert(QLatin1String("ProjectExplorer.Project.EditorSettings"),
                      editorSettingsMap);
    }

    QVariant toolchain(map.value("toolChain"));
    if (toolchain.isValid()) {
        bool ok;
        int type(toolchain.toInt(&ok));
        if (!ok) {
            QString toolChainName(toolchain.toString());
            if (toolChainName == QLatin1String("gcc"))
                type = ToolChain::GCC;
            else if (toolChainName == QLatin1String("mingw"))
                type = ToolChain::MinGW;
            else if (toolChainName == QLatin1String("msvc"))
                type = ToolChain::MSVC;
            else if (toolChainName == QLatin1String("wince"))
                type = ToolChain::WINCE;
        }
        result.insert(QLatin1String("GenericProjectManager.GenericProject.Toolchain"), type);
    }

    return result;
}

// -------------------------------------------------------------------------
// Version1Handler
// -------------------------------------------------------------------------

Version1Handler::Version1Handler()
{
}

Version1Handler::~Version1Handler()
{
}

QVariantMap Version1Handler::update(Project *project, const QVariantMap &map)
{
    QVariantMap result;

    // The only difference between version 1 and 2 of the user file is that
    // we need to add targets.

    // Generate a list of all possible targets for the project:
    QList<TargetDescription> targets;
    if (project->id() == QLatin1String("GenericProjectManager.GenericProject"))
        targets << TargetDescription(QString::fromLatin1("GenericProjectManager.GenericTarget"),
                                     QCoreApplication::translate("GenericProjectManager::GenericTarget",
                                                                 "Desktop",
                                                                 "Generic desktop target display name"));
    else if (project->id() == QLatin1String("CMakeProjectManager.CMakeProject"))
        targets << TargetDescription(QString::fromLatin1("CMakeProjectManager.DefaultCMakeTarget"),
                                     QCoreApplication::translate("CMakeProjectManager::Internal::CMakeTarget",
                                                                 "Desktop",
                                                                 "CMake Default target display name"));
    else if (project->id() == QLatin1String("Qt4ProjectManager.Qt4Project"))
        targets << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.DesktopTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Desktop",
                                                                 "Qt4 Desktop target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.S60EmulatorTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Symbian Emulator",
                                                                 "Qt4 Symbian Emulator target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.S60DeviceTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Symbian Device",
                                                                 "Qt4 Symbian Device target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.MaemoEmulatorTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Maemo Emulator",
                                                                 "Qt4 Maemo Emulator target display name"))
        << TargetDescription(QString::fromLatin1("Qt4ProjectManager.Target.MaemoDeviceTarget"),
                                     QCoreApplication::translate("Qt4ProjectManager::Internal::Qt4Target",
                                                                 "Maemo Device",
                                                                 "Qt4 Maemo Device target display name"));
    else if (project->id() == QLatin1String("QmlProjectManager.QmlProject"))
        targets << TargetDescription(QString::fromLatin1("QmlProjectManager.QmlTarget"),
                                     QCoreApplication::translate("QmlProjectManager::QmlTarget",
                                                                 "QML Viewer",
                                                                 "Qml Viewer target display name"));
    else
        return QVariantMap(); // We do not know how to handle this.

    result.insert(QLatin1String("ProjectExplorer.Project.ActiveTarget"), 0);
    result.insert(QLatin1String("ProjectExplorer.Project.TargetCount"), targets.count());
    int pos(0);
    foreach (const TargetDescription &td, targets) {
        QVariantMap targetMap;
        // Do not set displayName or icon!
        targetMap.insert(QLatin1String("ProjectExplorer.ProjectConfiguration.Id"), td.id);

        int count = map.value(QLatin1String("ProjectExplorer.Project.BuildConfigurationCount")).toInt();
        targetMap.insert(QLatin1String("ProjectExplorer.Target.BuildConfigurationCount"), count);
        for (int i = 0; i < count; ++i) {
            QString key(QString::fromLatin1("ProjectExplorer.Project.BuildConfiguration.") + QString::number(i));
            if (map.contains(key))
                targetMap.insert(QString::fromLatin1("ProjectExplorer.Target.BuildConfiguration.") + QString::number(i),
                                 map.value(key));
        }

        count = map.value(QLatin1String("ProjectExplorer.Project.RunConfigurationCount")).toInt();
        for (int i = 0; i < count; ++i) {
            QString key(QString::fromLatin1("ProjectExplorer.Project.RunConfiguration.") + QString::number(i));
            if (map.contains(key))
                targetMap.insert(QString::fromLatin1("ProjectExplorer.Target.RunConfiguration.") + QString::number(i),
                                 map.value(key));
        }

        if (map.contains(QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration")))
            targetMap.insert(QLatin1String("ProjectExplorer.Target.ActiveBuildConfiguration"),
                             map.value(QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration")));
        if (map.contains(QLatin1String("ProjectExplorer.Project.ActiveRunConfiguration")))
            targetMap.insert(QLatin1String("ProjectExplorer.Target.ActiveRunConfiguration"),
                             map.value(QLatin1String("ProjectExplorer.Project.ActiveRunConfiguration")));
        if (map.contains(QLatin1String("ProjectExplorer.Project.RunConfigurationCount")))
            targetMap.insert(QLatin1String("ProjectExplorer.Target.RunConfigurationCount"),
                             map.value(QLatin1String("ProjectExplorer.Project.RunConfigurationCount")));

        result.insert(QString::fromLatin1("ProjectExplorer.Project.Target.") + QString::number(pos), targetMap);
        ++pos;
    }

    // copy everything else:
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i) {
        if (i.key() == QLatin1String("ProjectExplorer.Project.ActiveBuildConfiguration") ||
            i.key() == QLatin1String("ProjectExplorer.Project.BuildConfigurationCount") ||
            i.key() == QLatin1String("ProjectExplorer.Project.ActiveRunConfiguration") ||
            i.key() == QLatin1String("ProjectExplorer.Project.RunConfigurationCount") ||
            i.key().startsWith(QLatin1String("ProjectExplorer.Project.BuildConfiguration.")) ||
            i.key().startsWith(QLatin1String("ProjectExplorer.Project.RunConfiguration.")))
            continue;
        result.insert(i.key(), i.value());
    }

    return result;
}
