/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmlprofilerstatewidget.h"

#include <utils/qtcassert.h>
#include <utils/theme/theme.h>

#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTime>
#include <QDebug>
#include <QTimer>
#include <QPointer>

#include <functional>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStateWidget::QmlProfilerStateWidgetPrivate
{
    public:
    QmlProfilerStateWidgetPrivate(QmlProfilerStateWidget *qq) : text(nullptr) { Q_UNUSED(qq); }

    QLabel *text;

    QPointer<QmlProfilerStateManager> m_profilerState;
    QPointer<QmlProfilerModelManager> m_modelManager;
    QTimer timer;
};

QmlProfilerStateWidget::QmlProfilerStateWidget(QmlProfilerStateManager *stateManager,
                                QmlProfilerModelManager *modelManager, QWidget *parent)
    : QFrame(parent), d(new QmlProfilerStateWidgetPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler State Display"));
    setFrameStyle(QFrame::StyledPanel);

    // UI elements
    auto layout = new QVBoxLayout(this);
    resize(200,70);

    d->text = new QLabel(this);
    d->text->setAlignment(Qt::AlignCenter);
    setAutoFillBackground(true);
    layout->addWidget(d->text);

    setLayout(layout);

    // profiler state
    d->m_modelManager = modelManager;

    modelManager->registerFeatures(0, QmlProfilerModelManager::QmlEventLoader(),
                                   std::bind(&QmlProfilerStateWidget::initialize, this),
                                   std::bind(&QmlProfilerStateWidget::clear, this),
                                   std::bind(&QmlProfilerStateWidget::clear, this));
    d->m_profilerState = stateManager;
    connect(&d->timer, &QTimer::timeout, this, &QmlProfilerStateWidget::updateDisplay);

    d->timer.setInterval(1000);
    setVisible(false);
}

QmlProfilerStateWidget::~QmlProfilerStateWidget()
{
    delete d;
}

void QmlProfilerStateWidget::reposition()
{
    QWidget *parentWidget = qobject_cast<QWidget *>(parent());
    // positioning it at 2/3 height (it looks better)
    move(parentWidget->width()/2 - width()/2, parentWidget->height()/3 - height()/2);
}

void QmlProfilerStateWidget::initialize()
{
    connect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerStateWidget::updateDisplay);
    connect(d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            this, &QmlProfilerStateWidget::updateDisplay);
    d->timer.start();
    updateDisplay();
}

void QmlProfilerStateWidget::showText(const QString &text)
{
    setVisible(true);
    d->text->setText(text);
    resize(300, 70);
    reposition();
}

void QmlProfilerStateWidget::clear()
{
    disconnect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
               this, &QmlProfilerStateWidget::updateDisplay);
    disconnect(d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
               this, &QmlProfilerStateWidget::updateDisplay);
    d->timer.stop();
    setVisible(false);
}

void QmlProfilerStateWidget::updateDisplay()
{
    QTC_ASSERT(d->m_modelManager, return);
    QTC_ASSERT(d->m_profilerState, return);

    // When application is being profiled
    if (d->m_profilerState->serverRecording()) {
        // Heuristic to not show the number if the application will only send the events when it
        // stops. The number is still > 0 then because we get some StartTrace etc.
        const int numEvents = d->m_modelManager->numEvents();
        showText(numEvents > 256 ? tr("Profiling application: %n events", nullptr, numEvents) :
                                   tr("Profiling application"));
    } else if (d->m_modelManager->traceDuration() > 0 && d->m_modelManager->isEmpty()) {
        // After profiling, there is an empty trace
        showText(tr("No QML events recorded"));
    } else if (!d->m_modelManager->isEmpty()) {
        // When datamodel is acquiring data
        if (d->m_profilerState->currentState() != QmlProfilerStateManager::Idle) {
            // we don't know how much more, so progress numbers are strange here
            showText(tr("Loading buffered data: %n events", nullptr,
                        d->m_modelManager->numEvents()));
        } else {
            // Application died before all data could be read
            showText(tr("Loading offline data: %n events", nullptr,
                        d->m_modelManager->numEvents()));
        }
    } else {
        showText(tr("Waiting for data"));
    }
}

} // namespace Internal
} // namespace QmlProfiler
