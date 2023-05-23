// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../qmldesignerbase_global.h"

#include <QObject>
#include <QQmlPropertyMap>
#include <QtQuickWidgets/QQuickWidget>

class QMLDESIGNERBASE_EXPORT StudioPropertyMap : public QQmlPropertyMap
{
public:
    struct PropertyPair
    {
        QString name;
        QVariant value;
    };

    explicit StudioPropertyMap(QObject *parent = 0);

    void setProperties(const QList<PropertyPair> &properties);
};

class QMLDESIGNERBASE_EXPORT StudioQuickWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StudioQuickWidget(QWidget *parent = nullptr);

    QQmlEngine *engine() const;
    QQmlContext *rootContext() const;

    QQuickItem *rootObject() const;

    void setResizeMode(QQuickWidget::ResizeMode mode);

    void setSource(const QUrl &url);

    void refresh();

    void setClearColor(const QColor &color);

    QList<QQmlError> errors() const;

    StudioPropertyMap *registerPropertyMap(const QByteArray &name);
    QQuickWidget *quickWidget() const;

private:
    QQuickWidget *m_quickWidget = nullptr;
};
