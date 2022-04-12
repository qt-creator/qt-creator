/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "languageclient_global.h"

#include <languageserverprotocol/lsptypes.h>

#include <utils/id.h>

#include <QMap>
#include <QTextEdit>

#include <functional>

namespace TextEditor {
class TextDocument;
class TextMark;
}

namespace LanguageClient {

class Client;

class LANGUAGECLIENT_EXPORT DiagnosticManager : public QObject
{
    Q_OBJECT
public:
    explicit DiagnosticManager(Client *client);
    ~DiagnosticManager() override;

    virtual void setDiagnostics(const LanguageServerProtocol::DocumentUri &uri,
                                const QList<LanguageServerProtocol::Diagnostic> &diagnostics,
                                const Utils::optional<int> &version);

    virtual void showDiagnostics(const LanguageServerProtocol::DocumentUri &uri, int version);
    virtual void hideDiagnostics(const Utils::FilePath &filePath);
    virtual QList<LanguageServerProtocol::Diagnostic> filteredDiagnostics(
        const QList<LanguageServerProtocol::Diagnostic> &diagnostics) const;

    void clearDiagnostics();

    QList<LanguageServerProtocol::Diagnostic> diagnosticsAt(
        const LanguageServerProtocol::DocumentUri &uri,
        const QTextCursor &cursor) const;
    bool hasDiagnostic(const LanguageServerProtocol::DocumentUri &uri,
                       const TextEditor::TextDocument *doc,
                       const LanguageServerProtocol::Diagnostic &diag) const;

signals:
    void textMarkCreated(const Utils::FilePath &path);

protected:
    Client *client() const { return m_client; }
    virtual TextEditor::TextMark *createTextMark(const Utils::FilePath &filePath,
                                                 const LanguageServerProtocol::Diagnostic &diagnostic,
                                                 bool isProjectFile) const;
    virtual QTextEdit::ExtraSelection createDiagnosticSelection(
        const LanguageServerProtocol::Diagnostic &diagnostic, QTextDocument *textDocument) const;

    void setExtraSelectionsId(const Utils::Id &extraSelectionsId);

private:
    struct VersionedDiagnostics
    {
        Utils::optional<int> version;
        QList<LanguageServerProtocol::Diagnostic> diagnostics;
    };
    QMap<LanguageServerProtocol::DocumentUri, VersionedDiagnostics> m_diagnostics;
    QMap<Utils::FilePath, QList<TextEditor::TextMark *>> m_marks;
    Client *m_client;
    Utils::Id m_extraSelectionsId;
};

} // namespace LanguageClient
