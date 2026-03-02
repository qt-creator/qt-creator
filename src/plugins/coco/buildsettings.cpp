// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsettings.h"

#include "buildsettings.h"
#include "cmakemodificationfile.h"
#include "cocobuildstep.h"
#include "cococommon.h"
#include "cocopluginconstants.h"
#include "cocoprojectwidget.h"
#include "cocotr.h"
#include "globalsettings.h"
#include "modificationfile.h"
#include "qmakefeaturefile.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Coco::Internal {

class CocoCMakeSettings : public BuildSettings
{
public:
    explicit CocoCMakeSettings(BuildConfiguration *bc)
        : BuildSettings{m_featureFile, bc}
    {}

    void connectToProject(CocoProjectWidget *parent) const override;
    void read() override;
    bool validSettings() const override;
    void setCoverage(bool on) override;

    QString saveButtonText() const override;
    QString configChanges() const override;
    bool needsReconfigure() const override { return true; }
    void reconfigure() override;
    void stopReconfigure() override;

    QString projectDirectory() const override;
    void write(const QString &options, const QString &tweaks) override;

private:
    bool hasInitialCacheOption(const QStringList &args) const;
    QString initialCacheOption() const;
    void writeToolchainFile(const QString &internalPath);

    CMakeModificationFile m_featureFile;
};

void CocoCMakeSettings::connectToProject(CocoProjectWidget *parent) const
{
    connect(
        buildConfig()->buildSystem(),
        &BuildSystem::updated,
        parent,
        [parent, bs = buildConfig()->buildSystem()] { parent->buildSystemUpdated(bs); });
    connect(
        buildConfig()->buildSystem(),
        &BuildSystem::errorOccurred,
        parent,
        &CocoProjectWidget::configurationErrorOccurred);
}

void CocoCMakeSettings::read()
{
    setEnabled(false);
    m_featureFile.setFilePath(buildConfig());
    m_featureFile.read();
    setEnabled(true);
}

QString CocoCMakeSettings::initialCacheOption() const
{
    return QString("-C%1").arg(m_featureFile.nativePath());
}

bool CocoCMakeSettings::hasInitialCacheOption(const QStringList &args) const
{
    for (int i = 0; i < args.length(); ++i) {
        if (args[i] == "-C" && i + 1 < args.length() && args[i + 1] == m_featureFile.nativePath())
            return true;

        if (args[i] == initialCacheOption())
            return true;
    }

    return false;
}

bool CocoCMakeSettings::validSettings() const
{
    const QStringList args = buildConfig()->additionalArgs();
    return enabled() && m_featureFile.exists() && hasInitialCacheOption(args);
}

void CocoCMakeSettings::setCoverage(bool on)
{
    if (!enabled())
        return;

    QStringList values = buildConfig()->initialArgs();
    QStringList args = Utils::filtered(values, [&](const QString &option) {
        return !(option.startsWith("-C") && option.endsWith(featureFilenName()));
    });

    if (on)
        args << QString("-C%1").arg(m_featureFile.nativePath());

    buildConfig()->setInitialArgs(args);
}

QString CocoCMakeSettings::saveButtonText() const
{
    return Tr::tr("Save && Re-configure");
}

QString CocoCMakeSettings::configChanges() const
{
    return "<table><tbody>"
           + tableRow(Tr::tr("Additional CMake options:"), maybeQuote(initialCacheOption()))
           + tableRow(Tr::tr("Initial cache script:"), maybeQuote(featureFilePath()))
           + "</tbody></table>";
}

void CocoCMakeSettings::reconfigure()
{
    if (enabled())
        buildConfig()->reconfigure();
}

void Coco::Internal::CocoCMakeSettings::stopReconfigure()
{
    if (enabled())
        buildConfig()->stopReconfigure();
}

QString CocoCMakeSettings::projectDirectory() const
{
    if (enabled())
        return buildConfig()->project()->projectDirectory().path();
    else
        return "";
}

void CocoCMakeSettings::write(const QString &options, const QString &tweaks)
{
    m_featureFile.setOptions(options);
    m_featureFile.setTweaks(tweaks);
    m_featureFile.write();

    writeToolchainFile(":/cocoplugin/files/cocoplugin-gcc.cmake");
    writeToolchainFile(":/cocoplugin/files/cocoplugin-clang.cmake");
    writeToolchainFile(":/cocoplugin/files/cocoplugin-visualstudio.cmake");
}

void CocoCMakeSettings::writeToolchainFile(const QString &internalPath)
{
    const Utils::FilePath projectDirectory = buildConfig()->project()->projectDirectory();

    QFile internalFile{internalPath};
    QTC_CHECK(internalFile.open(QIODeviceBase::ReadOnly));
    const QByteArray internalContent = internalFile.readAll();

    const QString fileName = Utils::FilePath::fromString(internalPath).fileName();
    const Utils::FilePath toolchainPath{projectDirectory.pathAppended(fileName)};
    const QString toolchainNative = toolchainPath.nativePath();

    if (toolchainPath.exists()) {
        QFile currentFile{toolchainNative};
        QTC_CHECK(currentFile.open(QIODeviceBase::ReadOnly));

        QByteArray currentContent = currentFile.readAll();
        if (internalContent == currentContent)
            return;

        logSilently(Tr::tr("Overwrite file \"%1\".").arg(toolchainNative));
    } else
        logSilently(Tr::tr("Write file \"%1\".").arg(toolchainNative));

    QFile out{toolchainNative};
    QTC_CHECK(out.open(QIODeviceBase::WriteOnly));
    out.write(internalContent);
    out.close();
}

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
    buildConfig()->userEnvironmentChanges().modifyEnvironment(env, buildConfig()->macroExpander());
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
    buildConfig()->setUserEnvironmentChanges({diff, {}});
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
               Tr::tr("Additional qmake arguments:"),
               maybeQuote(configAssignment()) + " " + maybeQuote(pathAssignment()))
           + tableRow(
               Tr::tr("Build environment:"),
               maybeQuote(QString(featuresVar) + "=" + projectDirectory()))
           + tableRow(Tr::tr("Feature file:"), maybeQuote(featureFilePath())) + "</tbody></table>";
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

// Generic

bool BuildSettings::supportsBuildConfig(const BuildConfiguration &config)
{
    return config.id() == QmakeProjectManager::Constants::QMAKE_BC_ID
           || config.id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID;
}

BuildSettings *BuildSettings::createdFor(BuildConfiguration *buildConfig)
{
    if (buildConfig->id() == QmakeProjectManager::Constants::QMAKE_BC_ID)
        return new CocoQMakeSettings(buildConfig);
    if (buildConfig->id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID)
        return new CocoCMakeSettings(buildConfig);

    return nullptr;
}

BuildSettings::BuildSettings(ModificationFile &featureFile, BuildConfiguration *buildConfig)
    : QObject(buildConfig), m_featureFile{featureFile}
{
    // Do not use m_featureFile in the constructor; it may not yet be valid.
}

void BuildSettings::connectToBuildStep(CocoBuildStep *step) const
{
    connect(buildConfig()->buildSystem(), &BuildSystem::updated,
            step, &CocoBuildStep::buildSystemUpdated);
}

bool BuildSettings::enabled() const
{
    return m_enabled;
}

const QStringList &BuildSettings::options() const
{
    return m_featureFile.options();
}

const QStringList &BuildSettings::tweaks() const
{
    return m_featureFile.tweaks();
}

bool BuildSettings::hasTweaks() const
{
    return !m_featureFile.tweaks().isEmpty();
}

QString BuildSettings::featureFilenName() const
{
    return m_featureFile.fileName();
}

QString BuildSettings::featureFilePath() const
{
    return m_featureFile.nativePath();
}

void BuildSettings::provideFile()
{
    if (!m_featureFile.exists())
        write("", "");
}

QString BuildSettings::tableRow(const QString &name, const QString &value) const
{
    return QString("<tr><td><b>%1</b></td><td>%2</td></tr>").arg(name, value);
}

void BuildSettings::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

BuildConfiguration *BuildSettings::buildConfig() const
{
    return qobject_cast<BuildConfiguration *>(parent());
}

} // namespace Coco::Internal
