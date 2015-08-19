/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/

#ifndef TESTRESULTDELEGATE_H
#define TESTRESULTDELEGATE_H

#include "testresultmodel.h"

#include <QStyledItemDelegate>

namespace Autotest {
namespace Internal {

class TestResultDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TestResultDelegate(QObject *parent = 0);
    ~TestResultDelegate();

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

public slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    class LayoutPositions
    {
    public:
        LayoutPositions(QStyleOptionViewItemV4 &options, TestResultModel *model)
            : m_totalWidth(options.rect.width()),
              m_maxFileLength(model->maxWidthOfFileName(options.font)),
              m_maxLineLength(model->maxWidthOfLineNumber(options.font)),
              m_realFileLength(m_maxFileLength),
              m_top(options.rect.top()),
              m_bottom(options.rect.bottom())
        {
            m_typeAreaWidth = QFontMetrics(options.font).width(QLatin1String("XXXXXXXX"));
            int flexibleArea = lineAreaLeft() - textAreaLeft() - ITEM_SPACING;
            if (m_maxFileLength > flexibleArea / 2)
                m_realFileLength = flexibleArea / 2;
            m_fontHeight = QFontMetrics(options.font).height();
        }

        int top() const { return m_top + ITEM_MARGIN; }
        int left() const { return ITEM_MARGIN; }
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
                                              fileAreaLeft() - ITEM_SPACING, m_fontHeight); }
        QRect fileArea() const { return QRect(fileAreaLeft(), top(),
                                              lineAreaLeft() - ITEM_SPACING, m_fontHeight); }

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

        static const int ICON_SIZE = 16;
        static const int ITEM_MARGIN = 2;
        static const int ITEM_SPACING = 4;

    };
};

} // namespace Internal
} // namespace Autotest

#endif // TESTRESULTDELEGATE_H
