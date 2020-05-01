/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
#include "mesonprojectimporter.h"

#include <QLoggingCategory>

namespace {
static Q_LOGGING_CATEGORY(mInputLog, "qtc.meson.import", QtWarningMsg);
}

namespace MesonProjectManager {
namespace Internal {
MesonProjectImporter::MesonProjectImporter(const Utils::FilePath &path)
    : QtSupport::QtProjectImporter{path}
{}

QStringList MesonProjectImporter::importCandidates()
{
    //TODO, this can be done later
    return {};
}

QList<void *> MesonProjectImporter::examineDirectory(const Utils::FilePath &importPath) const
{
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
