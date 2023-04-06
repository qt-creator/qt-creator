// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace ProjectExplorer { class ToolChain; }

namespace Qnx::Internal {

class QnxSettingsPage final : public Core::IOptionsPage
{
public:
    QnxSettingsPage();
    ~QnxSettingsPage();

    static QList<ProjectExplorer::ToolChain *> autoDetect(
        const QList<ProjectExplorer::ToolChain *> &alreadyKnown);
};

} // Qnx::Internal
