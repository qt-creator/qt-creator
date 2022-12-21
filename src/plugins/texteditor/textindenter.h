// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "indenter.h"
#include "tabsettings.h"

QT_BEGIN_NAMESPACE
class QTextDocument;
class QTextCursor;
class QChar;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT TextIndenter : public Indenter
{
public:
    explicit TextIndenter(QTextDocument *doc);
    ~TextIndenter() override;

    int indentFor(const QTextBlock &block,
                  const TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;

    IndentationForBlock indentationForBlocks(const QVector<QTextBlock> &blocks,
                                             const TabSettings &tabSettings,
                                             int cursorPositionInEditor = -1) override;
    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) override;

    void indent(const QTextCursor &cursor,
                const QChar &typedChar,
                const TabSettings &tabSettings,
                int cursorPositionInEditor = -1) override;

    void reindent(const QTextCursor &cursor,
                  const TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) override;
    std::optional<TabSettings> tabSettings() const override;
};

} // namespace TextEditor
