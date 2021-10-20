/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <projectexplorer/project.h>

#include <utils/fileutils.h>

namespace QmlDesigner {
namespace GenerateCmake {
void generateMenuEntry();
void onGenerateCmakeLists();
bool writeFile(const Utils::FilePath &filePath, const QString &fileContent);
}
namespace GenerateCmakeLists {
void generateMainCmake(const Utils::FilePath &rootDir);
void generateSubdirCmake(const Utils::FilePath &dir);
QString generateModuleCmake(const Utils::FilePath &dir);
QStringList processDirectory(const Utils::FilePath &dir);
QStringList getSingletonsFromQmldirFile(const Utils::FilePath &filePath);
QStringList getDirectoryTreeQmls(const Utils::FilePath &dir);
QStringList getDirectoryTreeResources(const Utils::FilePath &dir);
void createCmakeFile(const Utils::FilePath &filePath, const QString &content);
bool isFileBlacklisted(const QString &fileName);
}
namespace GenerateEntryPoints {
bool generateEntryPointFiles(const Utils::FilePath &dir);
bool generateMainCpp(const Utils::FilePath &dir);
bool generateMainQml(const Utils::FilePath &dir);
}
}
