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

#include "selectabletexteditorwidget.h"
#include <texteditor/textdocument.h>

#include <QPainter>
#include <QTextBlock>

namespace DiffEditor {
namespace Internal {

SelectableTextEditorWidget::SelectableTextEditorWidget(Core::Id id, QWidget *parent)
    : TextEditorWidget(parent)
{
    setFrameStyle(QFrame::NoFrame);
    setupFallBackEditor(id);
}

SelectableTextEditorWidget::~SelectableTextEditorWidget()
{
}

static QList<DiffSelection> subtractSelection(
        const DiffSelection &minuendSelection,
        const DiffSelection &subtrahendSelection)
{
    // tha case that whole minuend is before the whole subtrahend
    if (minuendSelection.end >= 0 && minuendSelection.end <= subtrahendSelection.start)
        return QList<DiffSelection>() << minuendSelection;

    // the case that whole subtrahend is before the whole minuend
    if (subtrahendSelection.end >= 0 && subtrahendSelection.end <= minuendSelection.start)
        return QList<DiffSelection>() << minuendSelection;

    bool makeMinuendSubtrahendStart = false;
    bool makeSubtrahendMinuendEnd = false;

    if (minuendSelection.start < subtrahendSelection.start)
        makeMinuendSubtrahendStart = true;
    if (subtrahendSelection.end >= 0 && (subtrahendSelection.end < minuendSelection.end || minuendSelection.end < 0))
        makeSubtrahendMinuendEnd = true;

    QList<DiffSelection> diffList;
    if (makeMinuendSubtrahendStart)
        diffList << DiffSelection(minuendSelection.start, subtrahendSelection.start, minuendSelection.format);
    if (makeSubtrahendMinuendEnd)
        diffList << DiffSelection(subtrahendSelection.end, minuendSelection.end, minuendSelection.format);

    return diffList;
}

void SelectableTextEditorWidget::setSelections(const QMap<int, QList<DiffSelection> > &selections)
{
    m_diffSelections.clear();
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
        m_diffSelections.insert(it.key(), workingList);
    }
}

void SelectableTextEditorWidget::paintBlock(QPainter *painter,
                                            const QTextBlock &block,
                                            const QPointF &offset,
                                            const QVector<QTextLayout::FormatRange> &selections,
                                            const QRect &clipRect) const
{
    const int blockNumber = block.blockNumber();
    QList<DiffSelection> diffs = m_diffSelections.value(blockNumber);

    QVector<QTextLayout::FormatRange> newSelections;
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

} // namespace Internal
} // namespace DiffEditor
