// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractaiapi.h"

namespace QmlDesigner {

class AiResponse;

class ClaudeMessagesApi : public AbstractAiApi
{
public:
    void setRequestHeader(QNetworkRequest *networkRequest, const AiModelInfo &modelInfo) override;
    QByteArray createContent(const Data &data, const AiModelInfo &modelInfo) override;

    AiResponse interpretResponse(const QByteArray &response) override;
    bool accepts(const AiModelInfo &info) override;
};

} // namespace QmlDesigner
