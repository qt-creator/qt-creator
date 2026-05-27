// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flamegraphwidget.h"

#include "timelinetheme.h"

#include <utils/theme/theme.h>

#include <QAbstractItemModel>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWidget>
#include <QVBoxLayout>

namespace Timeline {

class FlameGraphWidget::FlameGraphWidgetPrivate
{
public:
    QQuickWidget *m_quickWidget = nullptr;
    QAbstractItemModel *m_model = nullptr;
};

FlameGraphWidget::FlameGraphWidget(QAbstractItemModel *model, const QUrl &qmlUrl,
                                   QWidget *parent)
    : QWidget(parent), d(new FlameGraphWidgetPrivate)
{
    d->m_model = model;
    d->m_quickWidget = new QQuickWidget(this);

    d->m_quickWidget->engine()->addImportPath(":/qt/qml/");
    TimelineTheme::setupTheme(d->m_quickWidget->engine());

    d->m_quickWidget->rootContext()->setContextProperty(QLatin1String("flameGraphModel"), model);
    d->m_quickWidget->setSource(qmlUrl);
    d->m_quickWidget->setClearColor(
            Utils::creatorColor(Utils::Theme::Timeline_BackgroundColor1));

    d->m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    d->m_quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(d->m_quickWidget);

    connect(d->m_quickWidget->rootObject(), SIGNAL(typeSelected(int)),
            this, SIGNAL(typeSelected(int)));
}

FlameGraphWidget::~FlameGraphWidget()
{
    delete d;
}

QAbstractItemModel *FlameGraphWidget::model() const
{
    return d->m_model;
}

void FlameGraphWidget::selectByTypeId(int typeId)
{
    if (QObject *root = d->m_quickWidget->rootObject())
        root->setProperty("selectedTypeId", typeId);
}

void FlameGraphWidget::resetRoot()
{
    if (QObject *root = d->m_quickWidget->rootObject())
        QMetaObject::invokeMethod(root, "resetRoot");
}

bool FlameGraphWidget::isZoomed() const
{
    QObject *root = d->m_quickWidget->rootObject();
    return root && root->property("zoomed").toBool();
}

} // namespace Timeline
