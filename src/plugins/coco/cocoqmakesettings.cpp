// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocoqmakesettings.h"

#include "buildsettings.h"
#include "cococommon.h"
#include "cocopluginconstants.h"
#include "cocoprojectwidget.h"
#include "cocotr.h"
#include "globalsettings.h"
#include "qmakefeaturefile.h"

#include <utils/commandline.h>
#include <utils/environment.h>

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QObject>
#include <QStringList>

using namespace ProjectExplorer;

namespace Coco::Internal {

class CocoQMakeSettings : public BuildSettings
{
public:
    explicit CocoQMakeSettings(BuildConfiguration *buildConfig)
        : BuildSettings{m_featureFile, buildConfig}
    {}

    void read() override;
    bool validSettings() const override;
    void setCoverage(bool on) override;

    QString saveButtonText() const override;
    QString configChanges() const override;
    QString projectDirectory() const override;
    void write(const QString &options, const QString &tweaks) override;

private:
    bool environmentSet() const;
    QString pathAssignment() const;
    const QStringList userArgumentList() const;
    Utils::Environment buildEnvironment() const;
    void setQMakeFeatures() const;
    bool cocoPathValid() const;

    QMakeFeatureFile m_featureFile;
};

void CocoQMakeSettings::read()
{
    setEnabled(false);
    m_featureFile.setFilePath(buildConfig());
    m_featureFile.read();
    setEnabled(true);
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

    return buildConfig()->initialArgs();
}

Utils::Environment CocoQMakeSettings::buildEnvironment() const
{
    if (!enabled())
        return Utils::Environment();

    Utils::Environment env = buildConfig()->environment();
    env.modify(buildConfig()->userEnvironmentChanges());
    return env;
}

void CocoQMakeSettings::setQMakeFeatures() const
{
    if (!enabled())
        return;

    Utils::Environment env = buildEnvironment();

    const QString projectDir = buildConfig()->project()->projectDirectory().nativePath();
    if (env.value(featuresVar) != projectDir) {
        // Bug in prependOrSet(): It does not recognize if QMAKEFEATURES contains a single path
        // without a colon and then appends it twice.
        env.prependOrSet(featuresVar, projectDir);
    }

    Utils::EnvironmentItems diff = buildConfig()->baseEnvironment().diff(env);
    buildConfig()->setUserEnvironmentChanges(diff);
}

bool CocoQMakeSettings::environmentSet() const
{
    if (!enabled())
        return true;

    const Utils::Environment env = buildEnvironment();
    const Utils::FilePath projectDir = buildConfig()->project()->projectDirectory();
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
    QStringList args;

    for (const QString &arg : buildConfig()->initialArgs()) {
        if (!arg.startsWith(pathAssignmentPrefix) && arg != configAssignment())
            args.append(arg);
    }

    if (on) {
        args.append(configAssignment());
        args.append(pathAssignment());

        setQMakeFeatures();
        m_featureFile.write();
    }

    buildConfig()->setInitialArgs(args);
}

QString CocoQMakeSettings::saveButtonText() const
{
    return Tr::tr("Save");
}

QString CocoQMakeSettings::configChanges() const
{
    return "<table><tbody>"
           + tableRow(
               Tr::tr("Additional qmake arguments: "),
               maybeQuote(configAssignment()) + " " + maybeQuote(pathAssignment()))
           + tableRow(
               Tr::tr("Build environment: "),
               maybeQuote(QString(featuresVar) + "=" + projectDirectory()))
           + tableRow(Tr::tr("Feature File: "), maybeQuote(featureFilePath())) + "</tbody></table>";
}

QString CocoQMakeSettings::projectDirectory() const
{
    if (enabled())
        return buildConfig()->project()->projectDirectory().nativePath();
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
    return pathAssignmentPrefix + cocoSettings().cocoPath().toUserOutput();
}

bool CocoQMakeSettings::cocoPathValid() const
{
    for (const QString &arg : buildConfig()->initialArgs()) {
        if (arg.startsWith(pathAssignmentPrefix) && arg != pathAssignment())
            return false;
    }

    return true;
}

BuildSettings *createCocoQMakeSettings(BuildConfiguration *bc)
{
    return new CocoQMakeSettings(bc);
}

} // namespace Coco::Internal
