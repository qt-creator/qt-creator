// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QString>

#include <functional>

namespace AcpClient::Internal {

class AcpChatWidget;

QString agentProjectInstructions(const QString &projectName,
                                 const Utils::FilePath &projectDirectory);
QString projectFormatsDescription();
QString agentProjectPrompt(const QString &instructions, const QString &description);
Utils::FilePath projectToOpen(const Utils::FilePath &directory);

void setupAgentProjectWizard(const std::function<AcpChatWidget *()> &showChat);

} // namespace AcpClient::Internal
