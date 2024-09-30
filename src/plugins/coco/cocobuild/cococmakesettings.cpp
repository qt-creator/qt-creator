// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cococmakesettings.h"

#include "cocobuild/cocoprojectwidget.h"
#include "cocotr.h"
#include "common.h"

#include <cmakeprojectmanager/cmakebuildconfiguration.h>
#include <cmakeprojectmanager/cmakebuildsystem.h>
#include <projectexplorer/target.h>

using namespace ProjectExplorer;
using namespace CMakeProjectManager;

namespace Coco::Internal {

CocoCMakeSettings::CocoCMakeSettings(Project *project)
    : BuildSettings{m_featureFile, project}
    , m_featureFile{project}
{}

CocoCMakeSettings::~CocoCMakeSettings() {}

void CocoCMakeSettings::connectToProject(CocoProjectWidget *parent) const
{
    connect(
        activeTarget(), &Target::buildSystemUpdated, parent, &CocoProjectWidget::buildSystemUpdated);
    connect(
        qobject_cast<CMakeProjectManager::CMakeBuildSystem *>(activeTarget()->buildSystem()),
        &CMakeProjectManager::CMakeBuildSystem::errorOccurred,
        parent,
        &CocoProjectWidget::configurationErrorOccurred);
}

void CocoCMakeSettings::read()
{
    setEnabled(false);
    if (Target *target = activeTarget()) {
        if ((m_buildConfig = qobject_cast<CMakeBuildConfiguration *>(
                 target->activeBuildConfiguration()))) {
            m_featureFile.setProjectDirectory(m_buildConfig->project()->projectDirectory());
            m_featureFile.read();
            setEnabled(true);
        }
    }
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
    return enabled() && m_featureFile.exists()
           && hasInitialCacheOption(m_buildConfig->additionalCMakeArguments());
}

void CocoCMakeSettings::setCoverage(bool on)
{
    if (!enabled())
        return;

    auto values = m_buildConfig->initialCMakeOptions();
    QStringList args = Utils::filtered(values, [&](const QString &option) {
        return !(option.startsWith("-C") && option.endsWith(featureFilenName()));
    });

    if (on)
        args << QString("-C%1").arg(m_featureFile.nativePath());

    m_buildConfig->setInitialCMakeArguments(args);
}

QString CocoCMakeSettings::saveButtonText() const
{
    return Tr::tr("Save && Re-configure");
}

QString CocoCMakeSettings::configChanges() const
{
    return "<table><tbody>"
           + tableRow("Additional CMake options: ", maybeQuote(initialCacheOption()))
           + tableRow("Initial cache script: ", maybeQuote(featureFilePath())) + "</tbody></table>";
}

void CocoCMakeSettings::reconfigure()
{
    if (!enabled())
        return;

    m_buildConfig->cmakeBuildSystem()->clearCMakeCache();
    m_buildConfig->updateInitialCMakeArguments();
    m_buildConfig->cmakeBuildSystem()->runCMake();
}

void Coco::Internal::CocoCMakeSettings::stopReconfigure()
{
    if (enabled())
        m_buildConfig->cmakeBuildSystem()->stopCMakeRun();
}

QString CocoCMakeSettings::projectDirectory() const
{
    if (enabled())
        return m_buildConfig->project()->projectDirectory().path();
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
    const Utils::FilePath projectDirectory = m_buildConfig->project()->projectDirectory();

    QFile internalFile{internalPath};
    internalFile.open(QIODeviceBase::ReadOnly);
    const QByteArray internalContent = internalFile.readAll();

    const QString fileName = Utils::FilePath::fromString(internalPath).fileName();
    const Utils::FilePath toolchainPath{projectDirectory.pathAppended(fileName)};
    const QString toolchainNative = toolchainPath.nativePath();

    if (toolchainPath.exists()) {
        QFile currentFile{toolchainNative};
        currentFile.open(QIODeviceBase::ReadOnly);

        QByteArray currentContent = currentFile.readAll();
        if (internalContent == currentContent)
            return;

        logSilently(Tr::tr("Overwrite file %1").arg(maybeQuote(toolchainNative)));
    } else
        logSilently(Tr::tr("Write file %1").arg(maybeQuote(toolchainNative)));

    QFile out{toolchainNative};
    out.open(QIODeviceBase::WriteOnly);
    out.write(internalContent);
    out.close();
}

} // namespace Coco::Internal
