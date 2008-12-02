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
#include "gdbmacrosbuildstep.h"
#include "qt4projectmanagerconstants.h"
#include "qt4project.h"
#include "qmakestep.h"
#include "makestep.h"

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

GdbMacrosBuildStep::GdbMacrosBuildStep(Qt4Project *project)
    : BuildStep(project), m_project(project), m_configWidget(new GdbMacrosBuildStepConfigWidget)
{

}

GdbMacrosBuildStep::~GdbMacrosBuildStep()
{
}

bool GdbMacrosBuildStep::init(const QString &buildConfiguration)
{
    m_buildDirectory = m_project->buildDirectory(buildConfiguration);
    m_qmake = m_project->qtVersion(buildConfiguration)->qmakeCommand();
    m_buildConfiguration = buildConfiguration;
    qDebug()<<"m_buildDirectory "<<m_buildDirectory;
    qDebug()<<"m_qmake "<<m_qmake;
    return true;
}

void GdbMacrosBuildStep::run(QFutureInterface<bool> & fi)
{
    // TODO CONFIG handling

    QString dumperPath = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>()
                ->resourcePath() + "/gdbmacros/";
    QStringList files;
    files << "gdbmacros.cpp"
          << "gdbmacros.pro";


    QString destDir = m_buildDirectory + "/qtc-gdbmacros/";
    QDir dir;
    dir.mkpath(destDir);
    foreach(const QString &file, files) {
        QFile destination(destDir + file);
        if (destination.exists())
            destination.remove();
        QFile::copy(dumperPath + file, destDir + file);
        qDebug()<<"copying"<<(dumperPath + file)<<" to "<< (destDir + file);
    }

    Qt4Project *qt4Project = static_cast<Qt4Project *>(project());

    QProcess qmake;
    qmake.setEnvironment(qt4Project->environment(m_buildConfiguration).toStringList());
    qmake.setWorkingDirectory(destDir);
    QStringList configarguments;
    QStringList makeArguments;

    // Find qmake step...
    QMakeStep *qmakeStep = qt4Project->qmakeStep();
    // Find out which configuration is used in this build configuration
    // and what kind of CONFIG we need to pass to qmake for that
    if (qmakeStep->value(m_buildConfiguration, "buildConfiguration").isValid()) {
        QtVersion::QmakeBuildConfig defaultBuildConfiguration = qt4Project->qtVersion(m_buildConfiguration)->defaultBuildConfig();
        QtVersion::QmakeBuildConfig projectBuildConfiguration = QtVersion::QmakeBuildConfig(qmakeStep->value(m_buildConfiguration, "buildConfiguration").toInt());
        if ((defaultBuildConfiguration & QtVersion::BuildAll) && !(projectBuildConfiguration & QtVersion::BuildAll))
            configarguments << "CONFIG-=debug_and_release";
        if (!(defaultBuildConfiguration & QtVersion::BuildAll) && (projectBuildConfiguration & QtVersion::BuildAll))
            configarguments << "CONFIG+=debug_and_release";
        if ((defaultBuildConfiguration & QtVersion::DebugBuild) && !(projectBuildConfiguration & QtVersion::DebugBuild))
            configarguments << "CONFIG+=release";
        if (!(defaultBuildConfiguration & QtVersion::DebugBuild) && (projectBuildConfiguration & QtVersion::DebugBuild))
            configarguments << "CONFIG+=debug";
        if (projectBuildConfiguration & QtVersion::BuildAll)
            makeArguments << (projectBuildConfiguration & QtVersion::DebugBuild ? "debug" : "release");

    } else {
        // Old style with CONFIG+=debug_and_release
        qDebug() << "OLD STYLE CONF";
        configarguments << "CONFIG+=debug_and_release";
        const MakeStep *ms = qt4Project->makeStep();
        QStringList makeargs = ms->value(m_buildConfiguration, "makeargs").toStringList();
        if (makeargs.contains("debug")) {
            makeArguments <<  "debug";
        } else if(makeargs.contains("release")) {
            makeArguments << "release";
        }
    }

    QString mkspec = qt4Project->qtVersion(m_buildConfiguration)->mkspec();
    qmake.start(m_qmake, QStringList()<<"-spec"<<mkspec<<configarguments<<"gdbmacros.pro");
    qmake.waitForFinished();
    qDebug()<<qmake.readAllStandardError()<<qmake.readAllStandardOutput();

    qmake.start(qt4Project->qtVersion(m_buildConfiguration)->makeCommand(), makeArguments);
    qmake.waitForFinished();
    qDebug()<<qmake.readAllStandardError()<<qmake.readAllStandardOutput();

    fi.reportResult(true);
}

QString GdbMacrosBuildStep::name()
{
    return Constants::GDBMACROSBUILDSTEP;
}

QString GdbMacrosBuildStep::displayName()
{
    return "Gdb Macros Build";
}

ProjectExplorer::BuildStepConfigWidget *GdbMacrosBuildStep::createConfigWidget()
{
    return new GdbMacrosBuildStepConfigWidget;
}

bool GdbMacrosBuildStep::immutable() const
{
    return false;
}

bool GdbMacrosBuildStepFactory::canCreate(const QString &name) const
{
    return name == Constants::GDBMACROSBUILDSTEP;
}

ProjectExplorer::BuildStep *GdbMacrosBuildStepFactory::create(ProjectExplorer::Project *pro, const QString &name) const
{
    Q_ASSERT(name == Constants::GDBMACROSBUILDSTEP);
    Qt4Project *qt4project = qobject_cast<Qt4Project *>(pro);
    Q_ASSERT(qt4project);
    return new GdbMacrosBuildStep(qt4project);
}

QStringList GdbMacrosBuildStepFactory::canCreateForProject(ProjectExplorer::Project *pro) const
{
    QStringList results;
    if (qobject_cast<QObject *>(pro))
        results <<  Constants::GDBMACROSBUILDSTEP;
    return results;
}

QString GdbMacrosBuildStepFactory::displayNameForName(const QString &name) const
{
    if (name == Constants::GDBMACROSBUILDSTEP)
        return "Gdb Macros Build";
    else
        return QString::null;
}

QString GdbMacrosBuildStepConfigWidget::displayName() const
{
    return "Gdb Macros Build";
}

void GdbMacrosBuildStepConfigWidget::init(const QString & /*buildConfiguration*/)
{
    // TODO
}
