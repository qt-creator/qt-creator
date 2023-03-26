// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonrpcmessages.h"
#include "languagefeatures.h"

namespace LanguageServerProtocol {

class PublishDiagnosticsParams : public JsonObject
{
public:
    using JsonObject::JsonObject;

    DocumentUri uri() const { return DocumentUri::fromProtocol(typedValue<QString>(uriKey)); }
    void setUri(const DocumentUri &uri) { insert(uriKey, uri); }

    QList<Diagnostic> diagnostics() const { return array<Diagnostic>(diagnosticsKey); }
    void setDiagnostics(const QList<Diagnostic> &diagnostics)
    { insertArray(diagnosticsKey, diagnostics); }

    std::optional<int> version() const { return optionalValue<int>(versionKey); }
    void setVersion(int version) { insert(versionKey, version); }
    void clearVersion() { remove(versionKey); }

    bool isValid() const override { return contains(diagnosticsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT PublishDiagnosticsNotification : public Notification<PublishDiagnosticsParams>
{
public:
    explicit PublishDiagnosticsNotification(const PublishDiagnosticsParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "textDocument/publishDiagnostics";
};

} // namespace LanguageClient
