// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

    virtual void setDiagnostics(const Utils::FilePath &filePath,
                                const QList<LanguageServerProtocol::Diagnostic> &diagnostics,
                                const std::optional<int> &version);

    virtual void showDiagnostics(const Utils::FilePath &filePath, int version);
    virtual void hideDiagnostics(const Utils::FilePath &filePath);
    virtual QList<LanguageServerProtocol::Diagnostic> filteredDiagnostics(
        const QList<LanguageServerProtocol::Diagnostic> &diagnostics) const;

    void disableDiagnostics(TextEditor::TextDocument *document);
    void clearDiagnostics();

    QList<LanguageServerProtocol::Diagnostic> diagnosticsAt(
        const Utils::FilePath &filePath,
        const QTextCursor &cursor) const;
    bool hasDiagnostic(const Utils::FilePath &filePath,
                       const TextEditor::TextDocument *doc,
                       const LanguageServerProtocol::Diagnostic &diag) const;
    bool hasDiagnostics(const TextEditor::TextDocument *doc) const;

signals:
    void textMarkCreated(const Utils::FilePath &path);

protected:
    Client *client() const { return m_client; }
    virtual TextEditor::TextMark *createTextMark(TextEditor::TextDocument *doc,
                                                 const LanguageServerProtocol::Diagnostic &diagnostic,
                                                 bool isProjectFile) const;
    virtual QTextEdit::ExtraSelection createDiagnosticSelection(
        const LanguageServerProtocol::Diagnostic &diagnostic, QTextDocument *textDocument) const;

    void setExtraSelectionsId(const Utils::Id &extraSelectionsId);

    void forAllMarks(std::function<void (TextEditor::TextMark *)> func);

private:
    struct VersionedDiagnostics
    {
        std::optional<int> version;
        QList<LanguageServerProtocol::Diagnostic> diagnostics;
    };
    QMap<Utils::FilePath, VersionedDiagnostics> m_diagnostics;
    class Marks
    {
    public:
        ~Marks();
        bool enabled = true;
        QList<TextEditor::TextMark *> marks;
    };
    QMap<Utils::FilePath, Marks> m_marks;
    Client *m_client;
    Utils::Id m_extraSelectionsId;
};

} // namespace LanguageClient
