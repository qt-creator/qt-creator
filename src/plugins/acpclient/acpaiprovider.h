// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acpsettings.h"
#include "chatpanel.h"

#include <acp/acpv2.h>

#include <utils/aiassistance.h>
#include <utils/filepath.h>
#include <utils/result.h>

#include <QMap>

#include <functional>

namespace AcpClient::Internal {

class AcpChatTab;
class AcpInspector;

class AcpPromptRequest
{
public:
    Utils::FilePath workingDirectory;
    QMap<QString, QString> configValues;
    QString prompt;
    QList<Utils::FilePath> files;
    QList<TextContext> textContexts;
    QList<ImageContext> imageContexts;
    std::function<void(const Utils::Result<> &)> onSent;
};

AcpPromptRequest acpPromptRequest(const Utils::AiRequest &request,
                                  const Utils::AiTargetSelection &selection);

QList<Utils::AiTargetParameter> aiTargetParameters(
    const QList<Acp::V2::SessionConfigOption> &configOptions);

void runHeadlessAcpRequest(const AcpSettings::ServerInfo &server,
                           const AcpPromptRequest &request,
                           AcpInspector *inspector,
                           Utils::AiResponse *response);

void rememberAgentConfigOptions(const QString &serverId,
                                const QList<Acp::V2::SessionConfigOption> &configOptions);

Utils::AiProvider *createAcpAiProvider(AcpInspector *inspector,
                                       const std::function<void(AcpChatTab *)> &showTab,
                                       const std::function<void()> &showChat);

} // namespace AcpClient::Internal
