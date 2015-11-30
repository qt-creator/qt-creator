/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    QMapIterator<int, QList<DiffSelection> > itBlock(selections);
    while (itBlock.hasNext()) {
        itBlock.next();

        const QList<DiffSelection> diffSelections = itBlock.value();
        QList<DiffSelection> workingList;
        for (int i = 0; i < diffSelections.count(); i++) {
            const DiffSelection &diffSelection = diffSelections.at(i);

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
        const int blockNumber = itBlock.key();
        QVector<QTextLayout::FormatRange> selList;
        for (int i = 0; i < workingList.count(); i++) {
            const DiffSelection &diffSelection = workingList.at(i);
            if (diffSelection.format) {
                QTextLayout::FormatRange formatRange;
                formatRange.start = diffSelection.start;
                if (formatRange.start < 0)
                    formatRange.start = 0;
                formatRange.length = diffSelection.end < 0
                        ? INT_MAX
                        : diffSelection.end - diffSelection.start;
                formatRange.format = *diffSelection.format;
                if (diffSelection.end < 0)
                    formatRange.format.setProperty(QTextFormat::FullWidthSelection, true);
                selList.append(formatRange);
            }
        }
        m_diffSelections.insert(blockNumber, workingList);
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
    for (int i = 0; i < diffs.count(); i++) {
        const DiffSelection &diffSelection = diffs.at(i);
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
