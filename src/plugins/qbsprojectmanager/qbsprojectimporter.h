// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qtsupport/qtprojectimporter.h>

namespace QbsProjectManager::Internal {

class QbsProjectImporter final : public QtSupport::QtProjectImporter
{
    Q_OBJECT

public:
    QbsProjectImporter(const Utils::FilePath &path);

private:
    Utils::FilePaths importCandidates() override;
    QList<void *> examineDirectory(const Utils::FilePath &importPath, QString *warningMessage) const override;
    bool matchKit(void *directoryData, const ProjectExplorer::Kit *k) const override;
    ProjectExplorer::Kit *createKit(void *directoryData) const override;
    const QList<ProjectExplorer::BuildInfo> buildInfoList(void *directoryData) const override;
    void deleteDirectoryData(void *directoryData) const override;
};

} // QbsProjectManager::Internal
