// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cocobuild/buildsettings.h"
#include "cocobuild/cmakemodificationfile.h"

#include <QObject>
#include <QStringList>

namespace CMakeProjectManager {
class CMakeBuildConfiguration;
class CMakeConfig;
}

namespace Coco::Internal {

class CocoProjectWidget;

class CocoCMakeSettings : public BuildSettings
{
    Q_OBJECT
public:
    explicit CocoCMakeSettings(ProjectExplorer::Project *project);
    ~CocoCMakeSettings() override;

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

    CMakeProjectManager::CMakeBuildConfiguration *m_buildConfig;
    CMakeModificationFile m_featureFile;
};

} // namespace Coco::Internal
