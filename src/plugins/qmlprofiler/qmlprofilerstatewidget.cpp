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

namespace QmlProfiler {
namespace Internal {

class QmlProfilerStateWidget::QmlProfilerStateWidgetPrivate
{
    public:
    QmlProfilerStateWidgetPrivate(QmlProfilerStateWidget *qq) { Q_UNUSED(qq); }

    QLabel *text;
    QProgressBar *progressBar;
    QPixmap shadowPic;

    QmlProfilerStateManager *m_profilerState;
    QmlProfilerModelManager *m_modelManager;
};

QmlProfilerStateWidget::QmlProfilerStateWidget(QmlProfilerStateManager *stateManager,
                                QmlProfilerModelManager *modelManager, QWidget *parent)
    : QWidget(parent), d(new QmlProfilerStateWidgetPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler State Display"));

    // UI elements
    QVBoxLayout *layout = new QVBoxLayout(this);
    resize(200,70);

    d->shadowPic.load(QLatin1String(":/timeline/dialog_shadow.png"));

    d->text = new QLabel(this);
    d->text->setAlignment(Qt::AlignCenter);
    layout->addWidget(d->text);

    d->progressBar = new QProgressBar(this);
    layout->addWidget(d->progressBar);
    d->progressBar->setMaximum(1000);
    d->progressBar->setVisible(false);

    setLayout(layout);

    // profiler state
    d->m_modelManager = modelManager;
    connect(d->m_modelManager, &QmlProfilerModelManager::stateChanged,
            this, &QmlProfilerStateWidget::updateDisplay);
    connect(d->m_modelManager, &QmlProfilerModelManager::progressChanged,
            this, &QmlProfilerStateWidget::updateDisplay);
    d->m_profilerState = stateManager;
    connect(d->m_profilerState, &QmlProfilerStateManager::stateChanged,
            this, &QmlProfilerStateWidget::updateDisplay);
    connect(d->m_profilerState, &QmlProfilerStateManager::serverRecordingChanged,
            this, &QmlProfilerStateWidget::updateDisplay);

    updateDisplay();
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

void QmlProfilerStateWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.save();

    // Shadow
    // there is no actual qpainter borderimage, hacking it here
    int borderWidth = 4;

    // topleft
    painter.drawPixmap(QRect(0, 0, borderWidth, borderWidth),
                      d->shadowPic,
                      QRect(0, 0, borderWidth, borderWidth));
    // topright
    painter.drawPixmap(QRect(width()-borderWidth, 0, borderWidth, borderWidth),
                      d->shadowPic,
                      QRect(d->shadowPic.width()-borderWidth, 0, borderWidth, borderWidth));
    // bottomleft
    painter.drawPixmap(QRect(0, height()-borderWidth, borderWidth, borderWidth),
                      d->shadowPic,
                      QRect(0, d->shadowPic.height()-borderWidth, borderWidth, borderWidth));
    // bottomright
    painter.drawPixmap(QRect(width()-borderWidth, height()-borderWidth, borderWidth, borderWidth),
                      d->shadowPic,
                      QRect(d->shadowPic.width()-borderWidth,
                            d->shadowPic.height()-borderWidth,
                            borderWidth,
                            borderWidth));
    // top
    painter.drawPixmap(QRect(borderWidth, 0, width()-2*borderWidth, borderWidth),
                      d->shadowPic,
                      QRect(borderWidth, 0, d->shadowPic.width()-2*borderWidth, borderWidth));
    // bottom
    painter.drawPixmap(QRect(borderWidth, height()-borderWidth, width()-2*borderWidth, borderWidth),
                      d->shadowPic,
                      QRect(borderWidth,
                            d->shadowPic.height()-borderWidth,
                            d->shadowPic.width()-2*borderWidth,
                            borderWidth));
    // left
    painter.drawPixmap(QRect(0, borderWidth, borderWidth, height()-2*borderWidth),
                      d->shadowPic,
                      QRect(0, borderWidth, borderWidth, d->shadowPic.height()-2*borderWidth));
    // right
    painter.drawPixmap(QRect(width()-borderWidth, borderWidth, borderWidth, height()-2*borderWidth),
                      d->shadowPic,
                      QRect(d->shadowPic.width()-borderWidth,
                            borderWidth,
                            borderWidth,
                            d->shadowPic.height()-2*borderWidth));
    // center
    painter.drawPixmap(QRect(borderWidth, borderWidth, width()-2*borderWidth, height()-2*borderWidth),
                      d->shadowPic,
                      QRect(borderWidth,
                            borderWidth,
                            d->shadowPic.width()-2*borderWidth,
                            d->shadowPic.height()-2*borderWidth));


    // Background
    painter.setBrush(Utils::creatorTheme()->color(Utils::Theme::BackgroundColorNormal));
    painter.drawRoundedRect(QRect(borderWidth, 0, width()-2*borderWidth, height()-borderWidth), 6, 6);

    // restore painter
    painter.restore();

}

void QmlProfilerStateWidget::showText(const QString &text, bool showProgress)
{
    setVisible(true);
    if (showProgress)
        d->progressBar->setValue(d->m_modelManager->progress() * 1000);
    d->progressBar->setVisible(showProgress);
    d->text->setText(text);
    resize(300, 70);
    reposition();
}

void QmlProfilerStateWidget::updateDisplay()
{
    // When application is being profiled
    if (d->m_profilerState->serverRecording()) {
        showText(tr("Profiling application"));
        return;
    }

    QmlProfilerModelManager::State state = d->m_modelManager->state();
    if (state == QmlProfilerModelManager::Done || state == QmlProfilerModelManager::Empty) {
        // After profiling, there is an empty trace
        if (d->m_modelManager->traceTime()->duration() > 0 &&
                (d->m_modelManager->isEmpty() || d->m_modelManager->progress() == 0)) {
            showText(tr("No QML events recorded"));
            return;
        }
    } else if (d->m_modelManager->progress() != 0 && !d->m_modelManager->isEmpty()) {
        // When datamodel is acquiring or processing data
        if (state == QmlProfilerModelManager::ProcessingData) {
            showText(tr("Processing data"), true);
        } else  if (d->m_profilerState->currentState() != QmlProfilerStateManager::Idle) {
            if (state == QmlProfilerModelManager::AcquiringData) {
                // we don't know how much more, so progress numbers are strange here
                showText(tr("Waiting for more data"));
            } else if (state == QmlProfilerModelManager::ClearingData) {
                // when starting a second recording from the same process without aggregation
                showText(tr("Clearing old trace"));
            }
        } else if (state == QmlProfilerModelManager::AcquiringData) {
            // Application died before all data could be read
            showText(tr("Application stopped before loading all data"));
        } else if (state == QmlProfilerModelManager::ClearingData) {
            showText(tr("Clearing old trace"));
        }
        return;
    } else if (state == QmlProfilerModelManager::AcquiringData) {
        showText(tr("Waiting for data"));
        return;
    }

    // There is a trace on view, hide this dialog
    d->progressBar->setVisible(false);
    setVisible(false);
}

}
}
