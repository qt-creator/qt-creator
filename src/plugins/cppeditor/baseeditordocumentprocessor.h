// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseeditordocumentparser.h"
#include "cppcursorinfo.h"
#include "cppeditor_global.h"
#include "cppsemanticinfo.h"
#include "cpptoolsreuse.h"

#include <coreplugin/helpitem.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/quickfix.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <cplusplus/CppDocument.h>

#include <QTextEdit>
#include <QVariant>

#include <functional>

QT_BEGIN_NAMESPACE
template <typename T>
class QPromise;
QT_END_NAMESPACE

namespace TextEditor { class TextDocument; }

namespace CppEditor {

// For clang code model only, move?
struct CPPEDITOR_EXPORT ToolTipInfo
{
    QString text;
    QString briefComment;

    QStringList qDocIdCandidates;
    QString qDocMark;
    Core::HelpItem::Category qDocCategory;
    QVariant value;

    QString sizeInBytes;
};

class CPPEDITOR_EXPORT BaseEditorDocumentProcessor : public QObject
{
    Q_OBJECT

public:
    BaseEditorDocumentProcessor(QTextDocument *textDocument, const Utils::FilePath &filePath);
    ~BaseEditorDocumentProcessor() override;

    void run(bool projectsUpdated = false);
    virtual void semanticRehighlight() = 0;
    virtual void recalculateSemanticInfoDetached(bool force) = 0;
    virtual SemanticInfo recalculateSemanticInfo() = 0;
    virtual CPlusPlus::Snapshot snapshot() = 0;
    virtual BaseEditorDocumentParser::Ptr parser() = 0;
    virtual bool isParserRunning() const = 0;

    virtual TextEditor::QuickFixOperations
    extraRefactoringOperations(const TextEditor::AssistInterface &assistInterface);

    virtual void invalidateDiagnostics();

    virtual void setParserConfig(const BaseEditorDocumentParser::Configuration &config);

    virtual QFuture<CursorInfo> cursorInfo(const CursorInfoParams &params) = 0;

    const Utils::FilePath &filePath() const { return m_filePath; }

signals:
    // Signal interface to implement
    void projectPartInfoUpdated(const ProjectPartInfo &projectPartInfo);

    void codeWarningsUpdated(unsigned revision,
                             const QList<QTextEdit::ExtraSelection> &selections,
                             const TextEditor::RefactorMarkers &refactorMarkers);

    void ifdefedOutBlocksUpdated(unsigned revision,
                                 const QList<TextEditor::BlockRange> &ifdefedOutBlocks);

    void cppDocumentUpdated(const CPlusPlus::Document::Ptr document);    // TODO: Remove me
    void semanticInfoUpdated(const SemanticInfo semanticInfo); // TODO: Remove me

protected:
    static void runParser(QPromise<void> &promise,
                          BaseEditorDocumentParser::Ptr parser,
                          BaseEditorDocumentParser::UpdateParams updateParams);

    // Convenience
    unsigned revision() const { return static_cast<unsigned>(m_textDocument->revision()); }
    QTextDocument *textDocument() const { return m_textDocument; }

private:
    virtual void runImpl(const BaseEditorDocumentParser::UpdateParams &updateParams) = 0;

private:
    Utils::FilePath m_filePath;
    QTextDocument *m_textDocument;
};

} // namespace CppEditor
