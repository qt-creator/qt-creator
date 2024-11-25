// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    explicit TestResultDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void clearCache();
    void setShowDuration(bool showDuration) { m_showDuration = showDuration; }

private:
    void limitTextOutput(QString &output) const;
    void recalculateTextLayout(const QModelIndex &index, const QString &output,
                               const QFont &font, int width) const;

    mutable QModelIndex m_lastProcessedIndex;
    mutable QFont m_lastProcessedFont;
    mutable QTextLayout m_lastCalculatedLayout;
    mutable int m_lastCalculatedHeight = 0;
    mutable int m_lastWidth = -1;
    bool m_showDuration = true;

    class LayoutPositions
    {
    public:
        LayoutPositions(QStyleOptionViewItem &options, const TestResultFilterModel *filterModel,
                        bool showDuration)
            : m_top(options.rect.top()),
              m_left(options.rect.left()),
              m_right(options.rect.right()),
              m_showDuration(showDuration)
        {
            TestResultModel *srcModel = static_cast<TestResultModel *>(filterModel->sourceModel());
            m_maxFileLength = srcModel->maxWidthOfFileName(options.font);
            m_maxLineLength = srcModel->maxWidthOfLineNumber(options.font);
            m_realFileLength = m_maxFileLength;
            m_typeAreaWidth = QFontMetrics(options.font).horizontalAdvance("XXXXXXXX");
            m_durationAreaWidth = QFontMetrics(options.font).horizontalAdvance("XXXXXXXX ms");

            int flexibleArea = (m_showDuration ? durationAreaLeft() : fileAreaLeft())
                    - textAreaLeft() - ItemSpacing;
            if (m_maxFileLength > flexibleArea / 2)
                m_realFileLength = flexibleArea / 2;
            m_fontHeight = QFontMetrics(options.font).height();
        }

        int top() const { return m_top + ItemMargin; }
        int left() const { return m_left + ItemMargin; }
        int right() const { return m_right - ItemMargin; }
        int minimumHeight() const { return IconSize + 2 * ItemMargin; }

        int iconSize() const { return IconSize; }
        int typeAreaLeft() const { return left() + IconSize + ItemSpacing; }
        int textAreaLeft() const { return typeAreaLeft() + m_typeAreaWidth + ItemSpacing; }
        int textAreaWidth() const
        {
            if (m_showDuration)
                return durationAreaLeft() - 3 * ItemSpacing - textAreaLeft();
            return fileAreaLeft() - ItemSpacing - textAreaLeft();
        }
        int durationAreaLeft() const {  return fileAreaLeft() - 3 * ItemSpacing - m_durationAreaWidth; }
        int durationAreaWidth() const { return m_durationAreaWidth; }
        int fileAreaLeft() const { return lineAreaLeft() - ItemSpacing - m_realFileLength; }
        int lineAreaLeft() const { return right() - m_maxLineLength; }

        QRect textArea() const { return QRect(textAreaLeft(), top(),
                                              textAreaWidth(), m_fontHeight); }
        QRect durationArea() const { return QRect(durationAreaLeft(), top(),
                                                  durationAreaWidth(), m_fontHeight); }
        QRect fileArea() const { return QRect(fileAreaLeft(), top(),
                                              m_realFileLength + ItemSpacing, m_fontHeight); }

        QRect lineArea() const { return QRect(lineAreaLeft(), top(),
                                              m_maxLineLength, m_fontHeight); }

    private:
        int m_maxFileLength;
        int m_maxLineLength;
        int m_realFileLength;
        int m_top;
        int m_left;
        int m_right;
        int m_fontHeight;
        int m_typeAreaWidth;
        int m_durationAreaWidth;
        bool m_showDuration;

        static constexpr int IconSize = 16;
        static constexpr int ItemMargin = 2;
        static constexpr int ItemSpacing = 4;

    };
};

} // namespace Internal
} // namespace Autotest
