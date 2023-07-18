// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

namespace TextEditor { class DisplaySettings; }

namespace DiffEditor::Internal {

class DiffSelection
{
public:
    QTextCharFormat *format = nullptr;
    int start = -1;
    int end = -1;
};

// block number, list of ranges
// DiffSelection.start - can be -1 (continues from the previous line)
// DiffSelection.end - can be -1 (spans to the end of line, even after the last character in line)
using DiffSelections = QMap<int, QList<DiffSelection>>;

class SelectableTextEditorWidget : public TextEditor::TextEditorWidget
{
    Q_OBJECT
public:
    SelectableTextEditorWidget(Utils::Id id, QWidget *parent = nullptr);
    ~SelectableTextEditorWidget() override;
    void setSelections(const DiffSelections &selections);

    void setDisplaySettings(const TextEditor::DisplaySettings &displaySettings) override;

    static DiffSelections polishedSelections(const DiffSelections &selections);
    static void setFoldingIndent(const QTextBlock &block, int indent);

private:
    void paintBlock(QPainter *painter,
                    const QTextBlock &block,
                    const QPointF &offset,
                    const QList<QTextLayout::FormatRange> &selections,
                    const QRect &clipRect) const override;

    DiffSelections m_diffSelections;
};

} // namespace DiffEditor::Internal
