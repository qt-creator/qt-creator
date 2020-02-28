/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    Utils::optional<TabSettings> tabSettings() const override;
};

} // namespace TextEditor
