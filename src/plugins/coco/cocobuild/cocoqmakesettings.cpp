// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocoqmakesettings.h"

#include "cocobuild/cocoprojectwidget.h"
#include "cocopluginconstants.h"
#include "cocotr.h"
#include "common.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qmakeprojectmanager/qmakestep.h>

using namespace ProjectExplorer;

namespace Coco::Internal {

CocoQMakeSettings::CocoQMakeSettings(Project *project)
    : BuildSettings{m_featureFile, project}
{}

CocoQMakeSettings::~CocoQMakeSettings() {}

void CocoQMakeSettings::read()
{
    setEnabled(false);
    if (Target *target = activeTarget()) {
        if ((m_buildConfig = qobject_cast<QmakeProjectManager::QmakeBuildConfiguration*>(target->activeBuildConfiguration()))) {
            if (BuildStepList *buildSteps = m_buildConfig->buildSteps()) {
                if ((m_qmakeStep = buildSteps->firstOfType<QmakeProjectManager::QMakeStep>())) {
                    m_featureFile.setProjectDirectory(m_buildConfig->project()->projectDirectory());
                    m_featureFile.read();
                    setEnabled(true);
                }
            }
        }
    }
}

QString configAssignment()
{
    static const QString assignment = QString("CONFIG+=") + Constants::PROFILE_NAME;
    return assignment;
}

static const char pathAssignmentPrefix[] = "COCOPATH=";
static const char featuresVar[] = "QMAKEFEATURES";

const QStringList CocoQMakeSettings::userArgumentList() const
{
    if (!enabled())
        return {};

    Utils::ProcessArgs::ConstArgIterator it{m_qmakeStep->userArguments.unexpandedArguments()};
    QStringList result;

    while (it.next()) {
        if (it.isSimple())
            result << it.value();
    }

    return result;
}

Utils::Environment CocoQMakeSettings::buildEnvironment() const
{
    if (!enabled())
        return Utils::Environment();

    Utils::Environment env = m_buildConfig->environment();
    env.modify(m_buildConfig->userEnvironmentChanges());
    return env;
}

void CocoQMakeSettings::setQMakeFeatures() const
{
    if (!enabled())
        return;

    Utils::Environment env = buildEnvironment();

    const QString projectDir = m_buildConfig->project()->projectDirectory().nativePath();
    if (env.value(featuresVar) != projectDir) {
        // Bug in prependOrSet(): It does not recognize if QMAKEFEATURES contains a single path
        // without a colon and then appends it twice.
        env.prependOrSet(featuresVar, projectDir);
    }

    Utils::EnvironmentItems diff = m_buildConfig->baseEnvironment().diff(env);
    m_buildConfig->setUserEnvironmentChanges(diff);
}

bool CocoQMakeSettings::environmentSet() const
{
    if (!enabled())
        return true;

    const Utils::Environment env = buildEnvironment();
    const Utils::FilePath projectDir = m_buildConfig->project()->projectDirectory();
    const QString nativeProjectDir = projectDir.nativePath();
    return env.value(featuresVar) == nativeProjectDir
           || env.value(featuresVar).startsWith(nativeProjectDir + projectDir.pathListSeparator());
}

bool CocoQMakeSettings::validSettings() const
{
    const bool configured = userArgumentList().contains(configAssignment());
    return enabled() && configured && environmentSet() && m_featureFile.exists()
           && cocoPathValid();
}

void CocoQMakeSettings::setCoverage(bool on)
{
    QString args = m_qmakeStep->userArguments.unexpandedArguments();
    Utils::ProcessArgs::ArgIterator it{&args};

    while (it.next()) {
        if (it.isSimple()) {
            const QString value = it.value();
            if (value.startsWith(pathAssignmentPrefix) || value == configAssignment())
                it.deleteArg();
        }
    }
    if (on) {
        it.appendArg(configAssignment());
        it.appendArg(pathAssignment());

        setQMakeFeatures();
        m_featureFile.write();
    }

    m_qmakeStep->userArguments.setArguments(args);
}

QString CocoQMakeSettings::saveButtonText() const
{
    return Tr::tr("Save");
}

QString CocoQMakeSettings::configChanges() const
{
    return "<table><tbody>"
           + tableRow(
               "Additional qmake arguments: ",
               maybeQuote(configAssignment()) + " " + maybeQuote(pathAssignment()))
           + tableRow(
               "Build environment: ", maybeQuote(QString(featuresVar) + "=" + projectDirectory()))
           + tableRow("Feature File: ", maybeQuote(featureFilePath())) + "</tbody></table>";
}

QString CocoQMakeSettings::projectDirectory() const
{
    if (enabled())
        return m_buildConfig->project()->projectDirectory().nativePath();
    else
        return "";
}

void CocoQMakeSettings::write(const QString &options, const QString &tweaks)
{
    m_featureFile.setOptions(options);
    m_featureFile.setTweaks(tweaks);
    m_featureFile.write();
}

QString CocoQMakeSettings::pathAssignment() const
{
    return pathAssignmentPrefix + m_coco.directory().toUserOutput();
}

bool CocoQMakeSettings::cocoPathValid() const
{
    Utils::ProcessArgs::ConstArgIterator it{m_qmakeStep->userArguments.unexpandedArguments()};

    while (it.next()) {
        if (it.isSimple()) {
            const QString value = it.value();
            if (value.startsWith(pathAssignmentPrefix) && value != pathAssignment())
                return false;
        }
    }
    return true;
}

} // namespace Coco::Internal
