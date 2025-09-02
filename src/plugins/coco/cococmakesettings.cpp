// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cococmakesettings.h"

#include "buildsettings.h"
#include "cmakemodificationfile.h"
#include "cococommon.h"
#include "cocoprojectwidget.h"
#include "cocotr.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/target.h>
#include <utils/algorithm.h>

#include <QObject>
#include <QStringList>

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

BuildSettings *createCocoCMakeSettings(BuildConfiguration *bc)
{
    return new CocoCMakeSettings(bc);
}

} // namespace Coco::Internal
