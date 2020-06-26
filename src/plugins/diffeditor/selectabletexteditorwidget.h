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

#include <texteditor/texteditor.h>

namespace DiffEditor {
namespace Internal {

class DiffSelection
{
public:
    DiffSelection() = default;
    DiffSelection(QTextCharFormat *f) : format(f) {}
    DiffSelection(int s, int e, QTextCharFormat *f) : start(s), end(e), format(f) {}

    int start = -1;
    int end = -1;
    QTextCharFormat *format = nullptr;
};

class SelectableTextEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT
public:
    SelectableTextEditorWidget(Utils::Id id, QWidget *parent = nullptr);
    ~SelectableTextEditorWidget() override;
    void setSelections(const QMap<int, QList<DiffSelection> > &selections);

    static void setFoldingIndent(const QTextBlock &block, int indent);

private:
    void paintBlock(QPainter *painter,
                    const QTextBlock &block,
                    const QPointF &offset,
                    const QVector<QTextLayout::FormatRange> &selections,
                    const QRect &clipRect) const override;

    // block number, list of ranges
    // DiffSelection.start - can be -1 (continues from the previous line)
    // DiffSelection.end - can be -1 (spans to the end of line, even after the last character in line)
    QMap<int, QList<DiffSelection> > m_diffSelections;
};

} // namespace Internal
} // namespace DiffEditor
