// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildsettings.h"
#include "qmakefeaturefile.h"
#include "settings/cocoinstallation.h"

#include <utils/commandline.h>
#include <utils/environment.h>

#include <QObject>
#include <QStringList>

namespace QmakeProjectManager {
class QMakeStep;
class QmakeBuildConfiguration;
}

namespace Coco::Internal {

class CocoProjectWidget;

class CocoQMakeSettings : public BuildSettings
{
    Q_OBJECT
public:
    explicit CocoQMakeSettings(ProjectExplorer::Project *project);
    ~CocoQMakeSettings() override;

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

    QmakeProjectManager::QmakeBuildConfiguration *m_buildConfig;
    QmakeProjectManager::QMakeStep *m_qmakeStep;

    QMakeFeatureFile m_featureFile;
    CocoInstallation m_coco;
};

} // namespace Coco::Internal
