// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "studioquickwidget.h"

#include <coreplugin/icore.h>

#include <QQuickItem>
#include <QVBoxLayout>
#include <QWindow>
#include <QtQml/QQmlEngine>

QQmlEngine *s_engine = nullptr;

StudioQuickWidget::StudioQuickWidget(QWidget *parent)
    : QWidget{parent}
{
    if (!s_engine)
        s_engine = new QQmlEngine;

    m_quickWidget = new QQuickWidget(s_engine, this);

    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_quickWidget);
}

QQmlEngine *StudioQuickWidget::engine() const
{
    return m_quickWidget->engine();
}

QQmlContext *StudioQuickWidget::rootContext() const
{
    return m_quickWidget->rootContext();
}

QQuickItem *StudioQuickWidget::rootObject() const
{
    return m_quickWidget->rootObject();
}

void StudioQuickWidget::setResizeMode(QQuickWidget::ResizeMode mode)
{
    m_quickWidget->setResizeMode(mode);
}

void StudioQuickWidget::setSource(const QUrl &url)
{
    m_quickWidget->setSource(url);

    if (rootObject()) {
        const auto windows = rootObject()->findChildren<QWindow *>();

        for (auto window : windows) {
            window->setTransientParent(Core::ICore::dialogParent()->windowHandle());
        }
    }
}

void StudioQuickWidget::refresh() {}

void StudioQuickWidget::setClearColor(const QColor &color)
{
    m_quickWidget->setClearColor(color);
}

QList<QQmlError> StudioQuickWidget::errors() const
{
    return m_quickWidget->errors();
}

StudioPropertyMap *StudioQuickWidget::registerPropertyMap(const QByteArray &name)
{
    StudioPropertyMap *map = new StudioPropertyMap(this);
    [[maybe_unused]] const int typeIndex = qmlRegisterSingletonType<StudioPropertyMap>(
        name.data(), 1, 0, name.data(), [map](QQmlEngine *, QJSEngine *) { return map; });
    return map;
}

QQuickWidget *StudioQuickWidget::quickWidget() const
{
    return m_quickWidget;
}

StudioPropertyMap::StudioPropertyMap(QObject *parent)
    : QQmlPropertyMap(parent)
{}

void StudioPropertyMap::setProperties(const QList<PropertyPair> &properties)
{
    for (const PropertyPair &pair : properties)
        insert(pair.name, pair.value);
}
