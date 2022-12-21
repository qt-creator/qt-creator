// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <QtQml>

#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE
class QPointF;
class QQuickItem;
QT_END_NAMESPACE

class Tooltip : public QObject
{
    Q_OBJECT

public:
    explicit Tooltip(QObject *parent = nullptr);

    Q_INVOKABLE void showText(QQuickItem *item, const QPointF &pos, const QString &text);
    Q_INVOKABLE void hideText();

    static void registerDeclarativeType();
};

QML_DECLARE_TYPE(Tooltip)

#endif // TOOLTIP_H
