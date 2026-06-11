// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "pathchooser.h"

#include <QStyledItemDelegate>

namespace Utils {

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

} // Utils
