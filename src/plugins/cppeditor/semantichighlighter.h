// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/futuresynchronizer.h>

#include <QFutureWatcher>
#include <QTextCharFormat>
#include <QVector>

#include <functional>
#include <memory>
#include <set>

namespace TextEditor {
class HighlightingResult;
class Parenthesis;
class TextDocument;
}

QT_BEGIN_NAMESPACE
class QTextBlock;
QT_END_NAMESPACE

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
    void handleHighlighterResults();
    void onHighlighterFinished();

    unsigned documentRevision() const;
    QVector<TextEditor::Parenthesis> getClearedParentheses(const QTextBlock &block);

private:
    TextEditor::TextDocument *m_baseTextDocument;

    unsigned m_revision = 0;
    QHash<int, QTextCharFormat> m_formatMap;
    std::set<int> m_seenBlocks;
    int m_nextResultToHandle = 0;
    int m_resultCount = 0;

    HighlightingRunner m_highlightingRunner;
    Utils::FutureSynchronizer m_futureSynchronizer; // Keep before m_watcher.
    std::unique_ptr<QFutureWatcher<TextEditor::HighlightingResult>> m_watcher;
};

} // namespace CppEditor
