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

#include "gdbmacrosbuildstep.h"

#include "makestep.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QFileInfo>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

GdbMacrosBuildStep::GdbMacrosBuildStep(Qt4Project *project)
    : BuildStep(project), m_project(project)
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
    return true;
}

void GdbMacrosBuildStep::run(QFutureInterface<bool> & fi)
{
    QStringList files;
    files << "gdbmacros.cpp" << "gdbmacros.pro"
          << "LICENSE.LGPL" << "LGPL_EXCEPTION.TXT";

    QVariant v = value("clean");
    if (v.isNull() || v.toBool() == false) {
        addToOutputWindow("<b>Creating gdb macros library...</b>");
        // Normal run
        QString dumperPath = Core::ICore::instance()->resourcePath() + "/gdbmacros/";
        QString destDir = m_buildDirectory + "/qtc-gdbmacros/";
        QDir dir;
        dir.mkpath(destDir);
        foreach (const QString &file, files) {
            QString source = dumperPath + file;
            QString dest = destDir + file;
            QFileInfo destInfo(dest);
            if (destInfo.exists()) {
                if (destInfo.lastModified() >= QFileInfo(source).lastModified())
                    continue;
                QFile::remove(dest);
            }
            QFile::copy(source, dest);
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
            configarguments << "CONFIG+=debug_and_release";
            const MakeStep *ms = qt4Project->makeStep();
            QStringList makeargs = ms->value(m_buildConfiguration, "makeargs").toStringList();
            if (makeargs.contains("debug")) {
                makeArguments <<  "debug";
            } else if (makeargs.contains("release")) {
                makeArguments << "release";
            }
        }

        QString mkspec = qt4Project->qtVersion(m_buildConfiguration)->mkspec();
        qmake.start(m_qmake, QStringList()<<"-spec"<<mkspec<<configarguments<<"gdbmacros.pro");
        qmake.waitForFinished();

        QString makeCmd = qt4Project->qtVersion(m_buildConfiguration)->makeCommand();
        if (!value(m_buildConfiguration, "makeCmd").toString().isEmpty())
            makeCmd = value(m_buildConfiguration, "makeCmd").toString();
        if (!QFileInfo(makeCmd).isAbsolute()) {
            // Try to detect command in environment
            QString tmp = qt4Project->environment(m_buildConfiguration).searchInPath(makeCmd);
            makeCmd = tmp;
        }
        qmake.start(makeCmd, makeArguments);
        qmake.waitForFinished();

        fi.reportResult(true);
    } else {
        // Clean step, we want to remove the directory
        QString destDir = m_buildDirectory + "/qtc-gdbmacros/";
        Qt4Project *qt4Project = static_cast<Qt4Project *>(project());

        QProcess make;
        make.setEnvironment(qt4Project->environment(m_buildConfiguration).toStringList());
        make.setWorkingDirectory(destDir);
        make.start(qt4Project->qtVersion(m_buildConfiguration)->makeCommand(), QStringList()<<"distclean");
        make.waitForFinished();

        QStringList directories;
        directories << "debug"
                    << "release";

        foreach(const QString &file, files) {
            QFile destination(destDir + file);
            destination.remove();
        }

        foreach(const QString &dir, directories) {
            QDir destination(destDir + dir);
            destination.rmdir(destDir + dir);
        }

        QDir(destDir).rmdir(destDir);
        fi.reportResult(true);
    }
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
    if (qobject_cast<Qt4Project *>(pro))
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
