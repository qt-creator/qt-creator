// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QItemDelegate>
#include <QStyledItemDelegate>
#include <QTextLayout>

namespace Utils {

enum class HighlightingItemRole {
    LineNumber = Qt::UserRole,
    StartColumn,
    Length,
    Foreground,
    Background,
    User,
    DisplayExtra,
    DisplayExtraForeground
};

class QTCREATOR_UTILS_EXPORT AnnotatedItemDelegate : public QStyledItemDelegate
{
public:
    AnnotatedItemDelegate(QObject *parent = nullptr);
    ~AnnotatedItemDelegate() override;

    void setAnnotationRole(int role);
    int annotationRole() const;

    void setDelimiter(const QString &delimiter);
    const QString &delimiter() const;

protected:
    void paint(QPainter *painter,
                       const QStyleOptionViewItem &option,
                       const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    int m_annotationRole;
    QString m_delimiter;
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
                     const QString &text, const QList<QTextLayout::FormatRange> &format) const;
    QString m_tabString;
};

} // Utils
