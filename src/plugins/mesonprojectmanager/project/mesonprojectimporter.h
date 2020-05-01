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
#pragma once

#include "exewrappers/mesonwrapper.h"

#include "projectexplorer/buildinfo.h"
#include "projectexplorer/kit.h"
#include "qtsupport/qtprojectimporter.h"

namespace MesonProjectManager {
namespace Internal {
class MesonProjectImporter final : public QtSupport::QtProjectImporter
{
public:
    MesonProjectImporter(const Utils::FilePath &path);
    QStringList importCandidates() final;

private:
    // importPath is an existing directory at this point!
    QList<void *> examineDirectory(const Utils::FilePath &importPath) const final;
    // will get one of the results from examineDirectory
    bool matchKit(void *directoryData, const ProjectExplorer::Kit *k) const final;
    // will get one of the results from examineDirectory
    ProjectExplorer::Kit *createKit(void *directoryData) const final;
    // will get one of the results from examineDirectory
    const QList<ProjectExplorer::BuildInfo> buildInfoList(void *directoryData) const final;

    virtual void deleteDirectoryData(void *directoryData) const final;
};
} // namespace Internal
} // namespace MesonProjectManager
