// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmakestep.h"

#include <qtsupport/qtprojectimporter.h>

namespace QmakeProjectManager {
namespace Internal {

// Documentation inside.
class QmakeProjectImporter : public QtSupport::QtProjectImporter
{
public:
    QmakeProjectImporter(const Utils::FilePath &path);

    QStringList importCandidates() final;

private:
    QList<void *> examineDirectory(const Utils::FilePath &importPath, QString *warningMessage) const final;
    bool matchKit(void *directoryData, const ProjectExplorer::Kit *k) const final;
    ProjectExplorer::Kit *createKit(void *directoryData) const final;
    const QList<ProjectExplorer::BuildInfo> buildInfoList(void *directoryData) const final;

    void deleteDirectoryData(void *directoryData) const final;

    ProjectExplorer::Kit *createTemporaryKit(const QtProjectImporter::QtVersionData &data,
                                             const QString &parsedSpec,
                                             const QMakeStepConfig::OsType &osType) const;
};

} // namespace Internal
} // namespace QmakeProjectManager
