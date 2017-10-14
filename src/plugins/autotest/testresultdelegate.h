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

#include "testresultmodel.h"

#include <QStyledItemDelegate>
#include <QTextLayout>

namespace Autotest {
namespace Internal {

class TestResultDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TestResultDelegate(QObject *parent = 0);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void clearCache();

private:
    void recalculateTextLayout(const QModelIndex &index, const QString &output,
                               const QFont &font, int width) const;

    mutable QModelIndex m_lastProcessedIndex;
    mutable QFont m_lastProcessedFont;
    mutable QTextLayout m_lastCalculatedLayout;
    mutable int m_lastCalculatedHeight;

    class LayoutPositions
    {
    public:
        LayoutPositions(QStyleOptionViewItem &options, TestResultFilterModel *filterModel)
            : m_totalWidth(options.rect.width()),
              m_top(options.rect.top()),
              m_bottom(options.rect.bottom())
        {
            TestResultModel *srcModel = static_cast<TestResultModel *>(filterModel->sourceModel());
            m_maxFileLength = srcModel->maxWidthOfFileName(options.font);
            m_maxLineLength = srcModel->maxWidthOfLineNumber(options.font);
            m_realFileLength = m_maxFileLength;
            m_typeAreaWidth = QFontMetrics(options.font).width("XXXXXXXX");
            m_indentation = options.widget ? options.widget->style()->pixelMetric(
                                                 QStyle::PM_TreeViewIndentation, &options) : 0;

            const QModelIndex &rootIndex = srcModel->rootItem()->index();
            QModelIndex parentIndex = filterModel->mapToSource(options.index).parent();
            m_level = 1;
            while (rootIndex != parentIndex) {
                ++m_level;
                parentIndex = parentIndex.parent();
            }
            int flexibleArea = lineAreaLeft() - textAreaLeft() - ITEM_SPACING;
            if (m_maxFileLength > flexibleArea / 2)
                m_realFileLength = flexibleArea / 2;
            m_fontHeight = QFontMetrics(options.font).height();
        }

        int top() const { return m_top + ITEM_MARGIN; }
        int left() const { return ITEM_MARGIN + m_indentation * m_level; }
        int right() const { return m_totalWidth - ITEM_MARGIN; }
        int bottom() const { return m_bottom; }
        int minimumHeight() const { return ICON_SIZE + 2 * ITEM_MARGIN; }

        int iconSize() const { return ICON_SIZE; }
        int fontHeight() const { return m_fontHeight; }
        int typeAreaLeft() const { return left() + ICON_SIZE + ITEM_SPACING; }
        int typeAreaWidth() const { return m_typeAreaWidth; }
        int textAreaLeft() const { return typeAreaLeft() + m_typeAreaWidth + ITEM_SPACING; }
        int textAreaWidth() const { return fileAreaLeft() - ITEM_SPACING - textAreaLeft(); }
        int fileAreaLeft() const { return lineAreaLeft() - ITEM_SPACING - m_realFileLength; }
        int lineAreaLeft() const { return right() - m_maxLineLength; }

        QRect typeArea() const { return QRect(typeAreaLeft(), top(),
                                              typeAreaWidth(), m_fontHeight); }
        QRect textArea() const { return QRect(textAreaLeft(), top(),
                                              textAreaWidth(), m_fontHeight); }
        QRect fileArea() const { return QRect(fileAreaLeft(), top(),
                                              m_realFileLength + ITEM_SPACING, m_fontHeight); }

        QRect lineArea() const { return QRect(lineAreaLeft(), top(),
                                              m_maxLineLength, m_fontHeight); }

    private:
        int m_totalWidth;
        int m_maxFileLength;
        int m_maxLineLength;
        int m_realFileLength;
        int m_top;
        int m_bottom;
        int m_fontHeight;
        int m_typeAreaWidth;
        int m_level;
        int m_indentation;

        static const int ICON_SIZE = 16;
        static const int ITEM_MARGIN = 2;
        static const int ITEM_SPACING = 4;

    };
};

} // namespace Internal
} // namespace Autotest
