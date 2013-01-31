/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmlprofilerstatewidget.h"

#include <QPainter>

#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTime>

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
    QmlProfilerDataModel *m_profilerDataModel;

    bool isRecording;
    bool appKilled;
    bool emptyList;
    bool traceAvailable;
    bool loadingDone;
    QTime profilingTimer;
    qint64 estimatedProfilingTime;
};



QmlProfilerStateWidget::QmlProfilerStateWidget(QmlProfilerStateManager *stateManager,
        QmlProfilerDataModel *dataModel, QWidget *parent) :
    QWidget(parent), d(new QmlProfilerStateWidgetPrivate(this))
{
    setObjectName(QLatin1String("QML Profiler State Display"));

    // UI elements
    QVBoxLayout *layout = new QVBoxLayout(this);
    resize(200,70);

    d->shadowPic.load(QLatin1String(":/qmlprofiler/dialog_shadow.png"));

    d->text = new QLabel(this);
    d->text->setAlignment(Qt::AlignCenter);
    layout->addWidget(d->text);

    d->progressBar = new QProgressBar(this);
    layout->addWidget(d->progressBar);
    d->progressBar->setVisible(false);

    setLayout(layout);

    // internal state
    d->isRecording = false;
    d->appKilled = false;
    d->traceAvailable = false;
    d->loadingDone = true;
    d->emptyList = true;

    // profiler state
    d->m_profilerDataModel = dataModel;
    connect(d->m_profilerDataModel,SIGNAL(stateChanged()), this, SLOT(dataStateChanged()));
    connect(d->m_profilerDataModel,SIGNAL(countChanged()), this, SLOT(dataStateChanged()));
    d->m_profilerState = stateManager;
    connect(d->m_profilerState,SIGNAL(stateChanged()), this, SLOT(profilerStateChanged()));
    connect(d->m_profilerState, SIGNAL(serverRecordingChanged()),
            this, SLOT(profilerStateChanged()));

    updateDisplay();
    connect(parent,SIGNAL(resized()),this,SLOT(reposition()));
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
    painter.setBrush(QColor("#E0E0E0"));
    painter.setPen(QColor("#666666"));
    painter.drawRoundedRect(QRect(borderWidth, 0, width()-2*borderWidth, height()-borderWidth), 6, 6);

    // restore painter
    painter.restore();

}

void QmlProfilerStateWidget::updateDisplay()
{
    // When datamodel is acquiring data
    if (!d->loadingDone && !d->emptyList && !d->appKilled) {
        setVisible(true);
        d->text->setText(tr("Loading data"));
        if (d->isRecording) {
            d->isRecording = false;
            d->estimatedProfilingTime = d->profilingTimer.elapsed();
        }
        d->progressBar->setMaximum(d->estimatedProfilingTime);
        d->progressBar->setValue(d->m_profilerDataModel->lastTimeMark() * 1e-6);
        d->progressBar->setVisible(true);
        resize(300,70);
        reposition();
        return;
    }

    // When application is being profiled
    if (d->isRecording) {
        setVisible(true);
        d->progressBar->setVisible(false);
        d->text->setText(tr("Profiling application"));
        resize(200,70);
        reposition();
        return;
    }

    // After profiling, there is an empty trace
    if (d->traceAvailable && d->loadingDone && d->emptyList) {
        setVisible(true);
        d->progressBar->setVisible(false);
        d->text->setText(tr("No QML events recorded"));
        resize(200,70);
        reposition();
        return;
    }

    // Application died before all data could be read
    if (!d->loadingDone && !d->emptyList && d->appKilled) {
        setVisible(true);
        d->text->setText(tr("Application stopped before loading all data"));
        if (d->isRecording) {
            d->isRecording = false;
            d->estimatedProfilingTime = d->profilingTimer.elapsed();
        }
        d->progressBar->setMaximum(d->estimatedProfilingTime);
        d->progressBar->setValue(d->m_profilerDataModel->lastTimeMark() * 1e-6);
        d->progressBar->setVisible(true);
        resize(300,70);
        reposition();
        return;
    }

    // Everything empty (base state), commented out now but needed in the future.
//    if (d->emptyList && d->loadingDone) {
//        setVisible(true);
//        d->progressBar->setVisible(false);
//        d->text->setText(tr("Profiler ready"));
//        resize(200,70);
//        parentResized();
//        return;
//    }

    // There is a trace on view, hide this dialog
    setVisible(false);
}

void QmlProfilerStateWidget::dataStateChanged()
{
    d->loadingDone = d->m_profilerDataModel->currentState() == QmlProfilerDataModel::Done ||
            d->m_profilerDataModel->currentState() == QmlProfilerDataModel::Empty;
    d->traceAvailable = d->m_profilerDataModel->traceDuration() > 0;
    d->emptyList = d->m_profilerDataModel->count() == 0;
    updateDisplay();
}

void QmlProfilerStateWidget::profilerStateChanged()
{
    if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppKilled)
        d->appKilled = true;
    else
        if (d->m_profilerState->currentState() == QmlProfilerStateManager::AppStarting)
            d->appKilled = false;

    d->isRecording = d->m_profilerState->serverRecording();
    if (d->isRecording)
        d->profilingTimer.start();
    else
        d->estimatedProfilingTime = d->profilingTimer.elapsed();
    updateDisplay();
}

}
}
