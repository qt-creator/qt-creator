// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "selectabletexteditorwidget.h"

#include <texteditor/displaysettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/textdocumentlayout.h>
#include <texteditor/texteditorsettings.h>

#include <QPainter>
#include <QTextBlock>

using namespace TextEditor;

namespace DiffEditor::Internal {

SelectableTextEditorWidget::SelectableTextEditorWidget(Utils::Id id, QWidget *parent)
    : TextEditorWidget(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setupFallBackEditor(id);

    setReadOnly(true);

    connect(TextEditorSettings::instance(), &TextEditorSettings::displaySettingsChanged,
            this, &SelectableTextEditorWidget::setDisplaySettings);
    SelectableTextEditorWidget::setDisplaySettings(TextEditorSettings::displaySettings());

    setCodeStyle(TextEditorSettings::codeStyle());
    setCodeFoldingSupported(true);
}

SelectableTextEditorWidget::~SelectableTextEditorWidget() = default;

void SelectableTextEditorWidget::setSelections(const DiffSelections &selections)
{
    m_diffSelections = selections;
}

void SelectableTextEditorWidget::setDisplaySettings(const DisplaySettings &displaySettings)
{
    DisplaySettings settings = displaySettings;
    settings.m_textWrapping = false;
    settings.m_displayLineNumbers = true;
    settings.m_markTextChanges = false;
    settings.m_highlightBlocks = false;
    TextEditorWidget::setDisplaySettings(settings);
}

static QList<DiffSelection> subtractSelection(
        const DiffSelection &minuendSelection,
        const DiffSelection &subtrahendSelection)
{
    // tha case that whole minuend is before the whole subtrahend
    if (minuendSelection.end >= 0 && minuendSelection.end <= subtrahendSelection.start)
        return {minuendSelection};

    // the case that whole subtrahend is before the whole minuend
    if (subtrahendSelection.end >= 0 && subtrahendSelection.end <= minuendSelection.start)
        return {minuendSelection};

    bool makeMinuendSubtrahendStart = false;
    bool makeSubtrahendMinuendEnd = false;

    if (minuendSelection.start < subtrahendSelection.start)
        makeMinuendSubtrahendStart = true;
    if (subtrahendSelection.end >= 0 && (subtrahendSelection.end < minuendSelection.end || minuendSelection.end < 0))
        makeSubtrahendMinuendEnd = true;

    QList<DiffSelection> diffList;
    if (makeMinuendSubtrahendStart)
        diffList += {minuendSelection.format, minuendSelection.start, subtrahendSelection.start};
    if (makeSubtrahendMinuendEnd)
        diffList += {minuendSelection.format, subtrahendSelection.end, minuendSelection.end};

    return diffList;
}

DiffSelections SelectableTextEditorWidget::polishedSelections(const DiffSelections &selections)
{
    DiffSelections polishedSelections;
    for (auto it = selections.cbegin(), end = selections.cend(); it != end; ++it) {
        const QList<DiffSelection> diffSelections = it.value();
        QList<DiffSelection> workingList;
        for (const DiffSelection &diffSelection : diffSelections) {
            if (diffSelection.start == -1 && diffSelection.end == 0)
                continue;

            if (diffSelection.start == diffSelection.end && diffSelection.start >= 0)
                continue;

            int j = 0;
            while (j < workingList.count()) {
                const DiffSelection existingSelection = workingList.takeAt(j);
                const QList<DiffSelection> newSelection = subtractSelection(existingSelection, diffSelection);
                for (int k = 0; k < newSelection.count(); k++)
                    workingList.insert(j + k, newSelection.at(k));
                j += newSelection.count();
            }
            workingList.append(diffSelection);
        }
        polishedSelections.insert(it.key(), workingList);
    }
    return polishedSelections;
}

void SelectableTextEditorWidget::setFoldingIndent(const QTextBlock &block, int indent)
{
    if (TextBlockUserData *userData = TextDocumentLayout::userData(block))
         userData->setFoldingIndent(indent);
}

void SelectableTextEditorWidget::paintBlock(QPainter *painter,
                                            const QTextBlock &block,
                                            const QPointF &offset,
                                            const QList<QTextLayout::FormatRange> &selections,
                                            const QRect &clipRect) const
{
    const int blockNumber = block.blockNumber();
    const QList<DiffSelection> diffs = m_diffSelections.value(blockNumber);

    QList<QTextLayout::FormatRange> newSelections;
    for (const DiffSelection &diffSelection : diffs) {
        if (diffSelection.format) {
            QTextLayout::FormatRange formatRange;
            formatRange.start = qMax(0, diffSelection.start);
            const int end = diffSelection.end < 0
                    ? block.text().count() + 1
                    : qMin(block.text().count(), diffSelection.end);

            formatRange.length = end - formatRange.start;
            formatRange.format = *diffSelection.format;
            if (diffSelection.end < 0)
                formatRange.format.setProperty(QTextFormat::FullWidthSelection, true);
            newSelections.append(formatRange);
        }
    }
    newSelections += selections;

    TextEditorWidget::paintBlock(painter, block, offset, newSelections, clipRect);
}

} // namespace DiffEditor::Internal
