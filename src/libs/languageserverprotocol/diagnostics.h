/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    bool isValid(ErrorHierarchy *error) const override
    { return checkArray<Diagnostic>(error, diagnosticsKey); }
};

class LANGUAGESERVERPROTOCOL_EXPORT PublishDiagnosticsNotification : public Notification<PublishDiagnosticsParams>
{
public:
    explicit PublishDiagnosticsNotification(const PublishDiagnosticsParams &params);
    using Notification::Notification;
    constexpr static const char methodName[] = "textDocument/publishDiagnostics";
};

} // namespace LanguageClient
