// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtCore/qtconfigmacros.h>

QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Utils {
class FilePath;
class FilePaths;
}

namespace QtSupport::Internal {

void setupGettingStartedWelcomePage();
void openExampleProject(const Utils::FilePath &project, const Utils::FilePaths &toOpen,
                        const Utils::FilePath &mainFile, const Utils::FilePaths &dependencies,
                        const QUrl &docUrl);

} // QtSupport::Internal
