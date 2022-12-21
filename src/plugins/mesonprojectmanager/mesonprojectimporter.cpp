// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonprojectimporter.h"

#include <QLoggingCategory>

namespace MesonProjectManager {
namespace Internal {

static Q_LOGGING_CATEGORY(mInputLog, "qtc.meson.import", QtWarningMsg);

MesonProjectImporter::MesonProjectImporter(const Utils::FilePath &path)
    : QtSupport::QtProjectImporter{path}
{}

Utils::FilePaths MesonProjectImporter::importCandidates()
{
    //TODO, this can be done later
    return {};
}

QList<void *> MesonProjectImporter::examineDirectory(const Utils::FilePath &importPath,
                                                     QString *warningMessage) const
{
    Q_UNUSED(warningMessage)
    //TODO, this can be done later
    qCDebug(mInputLog()) << "examining build directory" << importPath.toUserOutput();
    QList<void *> data;
    return data;
}

bool MesonProjectImporter::matchKit(void *directoryData, const ProjectExplorer::Kit *k) const
{
    Q_UNUSED(directoryData)
    Q_UNUSED(k)
    //TODO, this can be done later
    return false;
}

ProjectExplorer::Kit *MesonProjectImporter::createKit(void *directoryData) const
{
    Q_UNUSED(directoryData)
    //TODO, this can be done later
    return nullptr;
}

const QList<ProjectExplorer::BuildInfo> MesonProjectImporter::buildInfoList(void *directoryData) const
{
    Q_UNUSED(directoryData)
    //TODO, this can be done later
    return {};
}

void MesonProjectImporter::deleteDirectoryData(void *directoryData) const
{
    Q_UNUSED(directoryData)
}

} // namespace Internal
} // namespace MesonProjectManager
