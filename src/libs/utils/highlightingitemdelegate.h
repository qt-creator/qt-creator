// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QItemDelegate>
#include <QTextLayout>

namespace Utils {

enum class HighlightingItemRole {
    LineNumber = Qt::UserRole,
    StartColumn,
    Length,
    Foreground,
    Background,
    User
};

class QTCREATOR_UTILS_EXPORT HighlightingItemDelegate : public QItemDelegate
{
public:
    HighlightingItemDelegate(int tabWidth, QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    void setTabWidth(int width);

private:
    int drawLineNumber(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect,
                       const QModelIndex &index) const;
    void drawText(QPainter *painter, const QStyleOptionViewItem &option,
                  const QRect &rect, const QModelIndex &index) const;
    using QItemDelegate::drawDisplay;
    void drawDisplay(QPainter *painter, const QStyleOptionViewItem &option, const QRect &rect,
                     const QString &text, const QVector<QTextLayout::FormatRange> &format) const;
    QString m_tabString;
};

} // namespace Utils
