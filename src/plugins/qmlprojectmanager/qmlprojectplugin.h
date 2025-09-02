// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

namespace Core { class IEditor; }

namespace QmlProjectManager::Internal {

bool qdsInstallationExists();
bool checkIfEditorIsuiQml(Core::IEditor *editor);
Utils::FilePath projectFilePath();
Utils::FilePaths rootCmakeFiles();
QString qtVersion(const Utils::FilePath &projectFilePath);
QString qdsVersion(const Utils::FilePath &projectFilePath);
void openInQds(const Utils::FilePath &filePath);
const QString readFileContents(const Utils::FilePath &filePath);

} // QmlProject::Internal
