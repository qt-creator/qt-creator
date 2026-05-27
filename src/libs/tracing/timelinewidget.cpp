// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "timelinewidget.h"

#include "timelinemodelaggregator.h"
#include "timelinetheme.h"
#include "timelinezoomcontrol.h"

#include <QEvent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickWidget>
#include <QSGRendererInterface>
#include <QString>
#include <QVBoxLayout>

namespace Timeline {

class TimelineWidget::TimelineWidgetPrivate
{
public:
    QQuickWidget *m_quickWidget = nullptr;
    TimelineModelAggregator *m_aggregator = nullptr;
    TimelineZoomControl *m_zoomControl = nullptr;
    QString m_currentFile;
    int m_currentLine = -1;
    int m_currentColumn = 0;
    int m_currentTypeId = -1;

    void updateCurrentState()
    {
        QQuickItem *root = m_quickWidget->rootObject();
        if (!root)
            return;
        m_currentFile = root->property("fileName").toString();
        m_currentLine = root->property("lineNumber").toInt();
        m_currentColumn = root->property("columnNumber").toInt();
        m_currentTypeId = root->property("typeId").toInt();
    }
};

TimelineWidget::TimelineWidget(TimelineModelAggregator *aggregator,
                               TimelineZoomControl *zoomControl,
                               QWidget *parent)
    : QWidget(parent), d(new TimelineWidgetPrivate)
{
    d->m_aggregator = aggregator;
    d->m_zoomControl = zoomControl;

    d->m_quickWidget = new QQuickWidget(this);
    d->m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    d->m_quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(170);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(d->m_quickWidget);

    d->m_quickWidget->engine()->addImportPath(":/qt/qml/");
    TimelineTheme::setupTheme(d->m_quickWidget->engine());
    d->m_quickWidget->rootContext()->setContextProperty(
            QLatin1String("timelineModelAggregator"), aggregator);
    d->m_quickWidget->rootContext()->setContextProperty(
            QLatin1String("zoomControl"), zoomControl);
    d->m_quickWidget->setSource(QUrl(QLatin1String("qrc:/qt/qml/QtCreator/Tracing/MainView.qml")));

    connect(aggregator, &QObject::destroyed, this, [this] { d->m_quickWidget->setSource({}); });
    connect(zoomControl, &QObject::destroyed, this, [this] { d->m_quickWidget->setSource({}); });

    connect(aggregator, &TimelineModelAggregator::updateCursorPosition, this, [this] {
        d->updateCurrentState();
        if (!d->m_currentFile.isEmpty())
            emit gotoSourceLocation(d->m_currentFile, d->m_currentLine, d->m_currentColumn);
        emit typeSelected(d->m_currentTypeId);
    });
}

TimelineWidget::~TimelineWidget()
{
    delete d;
}

bool TimelineWidget::isUsable() const
{
    const QSGRendererInterface::GraphicsApi api =
            d->m_quickWidget->quickWindow()->rendererInterface()->graphicsApi();
    return QSGRendererInterface::isApiRhiBased(api);
}

bool TimelineWidget::hasValidSelection() const
{
    QQuickItem *root = d->m_quickWidget->rootObject();
    return root && root->property("selectionRangeReady").toBool();
}

qint64 TimelineWidget::selectionStart() const
{
    return d->m_zoomControl->selectionStart();
}

qint64 TimelineWidget::selectionEnd() const
{
    return d->m_zoomControl->selectionEnd();
}

QString TimelineWidget::currentFile() const
{
    return d->m_currentFile;
}

int TimelineWidget::currentLine() const
{
    return d->m_currentLine;
}

int TimelineWidget::currentColumn() const
{
    return d->m_currentColumn;
}

int TimelineWidget::currentTypeId() const
{
    return d->m_currentTypeId;
}

void TimelineWidget::clear()
{
    QQuickItem *root = d->m_quickWidget->rootObject();
    if (root)
        QMetaObject::invokeMethod(root, "clear");
}

void TimelineWidget::selectByTypeId(int typeId)
{
    QQuickItem *root = d->m_quickWidget->rootObject();
    if (root)
        QMetaObject::invokeMethod(root, "selectByTypeId", Q_ARG(QVariant, QVariant(typeId)));
}

void TimelineWidget::selectByIndices(int modelIndex, int eventIndex)
{
    QQuickItem *root = d->m_quickWidget->rootObject();
    if (!root)
        return;
    QMetaObject::invokeMethod(root, "selectByIndices",
                              Q_ARG(QVariant, QVariant(modelIndex)),
                              Q_ARG(QVariant, QVariant(eventIndex)));
    d->updateCurrentState();
}

void TimelineWidget::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::EnabledChange) {
        QQuickItem *root = d->m_quickWidget->rootObject();
        if (root)
            root->setProperty("enabled", isEnabled());
    }
    QWidget::changeEvent(e);
}

} // namespace Timeline
