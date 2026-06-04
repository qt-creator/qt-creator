// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dynamiccapabilities.h"

#include <utils/jsonrpcinspector.h>

#include <languageserverprotocol/jsonrpcmessages.h>
#include <languageserverprotocol/servercapabilities.h>

namespace LanguageClient {

using LspLogMessage = Utils::JsonRpcLogMessage;

struct Capabilities
{
    LanguageServerProtocol::ServerCapabilities capabilities;
    DynamicCapabilities dynamicCapabilities;
};

class LspInspector : public Utils::JsonRpcInspector
{
public:
    LspInspector();

    void log(LspLogMessage::MessageSender sender,
             const QString &clientName,
             const LanguageServerProtocol::JsonRpcMessage &message);
    void clientInitialized(const QString &clientName,
                           const LanguageServerProtocol::ServerCapabilities &capabilities);
    void updateCapabilities(const QString &clientName,
                            const DynamicCapabilities &dynamicCapabilities);

    Capabilities capabilities(const QString &clientName) const;

private:
    QMap<QString, Capabilities> m_capabilities;
};

} // namespace LanguageClient
