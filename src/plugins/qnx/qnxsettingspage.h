// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace ProjectExplorer { class ToolChain; }

namespace Qnx::Internal {

QList<ProjectExplorer::ToolChain *> autoDetectHelper(
    const QList<ProjectExplorer::ToolChain *> &alreadyKnown);

void setupQnxSettingsPage(QObject *guard);

} // Qnx::Internal
