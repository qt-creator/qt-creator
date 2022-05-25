/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include <utils/filepath.h>

namespace QmlProjectManager {
namespace GenerateQmlProject {

class QmlProjectFileGenerator {
public:
    bool prepare(const Utils::FilePath &targetDir);
    bool prepareForUiQmlFile(const Utils::FilePath &uiFilePath);
    bool execute();

    const Utils::FilePath targetDir() const;
    const Utils::FilePath targetFile() const;

private:
    const QString createFilteredDirEntries(const QStringList &suffixes) const;
    const QString createDirArrayEntry(const QString &arrayName, const QStringList &relativePaths) const;
    const Utils::FilePath selectTargetFile(const Utils::FilePath &uiFilePath = {});
    bool isStandardStructure(const Utils::FilePath &projectDir) const;
    bool isDirAcceptable(const Utils::FilePath &dir, const Utils::FilePath &uiFile);
    const QString createContentDirEntries(const QString &containerName,
                                          const QStringList &suffixes) const;
    const Utils::FilePath findInDirTree(const Utils::FilePath &dir,
                                        const QStringList &suffixes,
                                        int currentSearchDepth = 0) const;
    const QStringList findContentDirs(const QStringList &suffixes) const;

private:
    Utils::FilePath m_targetDir;
    Utils::FilePath m_targetFile;
};

} // GenerateQmlProject
} // QmlProjectManager
