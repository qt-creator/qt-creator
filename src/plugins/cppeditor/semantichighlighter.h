// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <QFutureWatcher>
#include <QScopedPointer>
#include <QTextCharFormat>

#include <functional>

namespace TextEditor {
class HighlightingResult;
class TextDocument;
}

namespace CppEditor {

class CPPEDITOR_EXPORT SemanticHighlighter : public QObject
{
    Q_OBJECT

public:
    enum Kind {
        Unknown = 0,
        TypeUse,
        NamespaceUse,
        LocalUse,
        FieldUse,
        EnumerationUse,
        VirtualMethodUse,
        LabelUse,
        MacroUse,
        FunctionUse,
        PseudoKeywordUse,
        FunctionDeclarationUse,
        VirtualFunctionDeclarationUse,
        StaticFieldUse,
        StaticMethodUse,
        StaticMethodDeclarationUse,
        AngleBracketOpen,
        AngleBracketClose,
        DoubleAngleBracketClose,
        TernaryIf,
        TernaryElse,
    };

    using HighlightingRunner = std::function<QFuture<TextEditor::HighlightingResult> ()>;

public:
    explicit SemanticHighlighter(TextEditor::TextDocument *baseTextDocument);
    ~SemanticHighlighter() override;

    void setHighlightingRunner(HighlightingRunner highlightingRunner);
    void updateFormatMapFromFontSettings();

    void run();

private:
    void onHighlighterResultAvailable(int from, int to);
    void onHighlighterFinished();

    void connectWatcher();
    void disconnectWatcher();

    unsigned documentRevision() const;

private:
    TextEditor::TextDocument *m_baseTextDocument;

    unsigned m_revision = 0;
    QScopedPointer<QFutureWatcher<TextEditor::HighlightingResult>> m_watcher;
    QHash<int, QTextCharFormat> m_formatMap;

    HighlightingRunner m_highlightingRunner;
};

} // namespace CppEditor
