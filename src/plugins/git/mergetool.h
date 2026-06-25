// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QStringList>

namespace Git::Internal {

void startMergeTool(const Utils::FilePath &workingDirectory, const QStringList &files);

} // Git::Internal
