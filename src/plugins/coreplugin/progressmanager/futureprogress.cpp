/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "futureprogress.h"
#include "progresspie.h"

#include <QtGui/QColor>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>

using namespace Core;

FutureProgress::FutureProgress(QWidget *parent)
        : QWidget(parent),
        m_progress(new ProgressBar),
        m_widget(0),
        m_widgetLayout(new QHBoxLayout)
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->addWidget(m_progress);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addLayout(m_widgetLayout);
    m_widgetLayout->setContentsMargins(7, 0, 7, 0);
    m_widgetLayout->setSpacing(0);

    connect(&m_watcher, SIGNAL(started()), this, SLOT(setStarted()));
    connect(&m_watcher, SIGNAL(finished()), this, SLOT(setFinished()));
    connect(&m_watcher, SIGNAL(progressRangeChanged(int,int)), this, SLOT(setProgressRange(int,int)));
    connect(&m_watcher, SIGNAL(progressValueChanged(int)), this, SLOT(setProgressValue(int)));
    connect(&m_watcher, SIGNAL(progressTextChanged(const QString&)),
            this, SLOT(setProgressText(const QString&)));
    connect(m_progress, SIGNAL(clicked()), this, SLOT(cancel()));
}

FutureProgress::~FutureProgress()
{
    if (m_widget)
        delete m_widget;
}

void FutureProgress::setWidget(QWidget *widget)
{
    if (m_widget)
        delete m_widget;
    QSizePolicy sp = widget->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Ignored);
    widget->setSizePolicy(sp);
    m_widget = widget;
    if (m_widget)
        m_widgetLayout->addWidget(m_widget);
}

void FutureProgress::setTitle(const QString &title)
{
    m_progress->setTitle(title);
}

QString FutureProgress::title() const
{
    return m_progress->title();
}

void FutureProgress::cancel()
{
    m_watcher.future().cancel();
}

void FutureProgress::setStarted()
{
    m_progress->reset();
    m_progress->setError(false);
    m_progress->setRange(m_watcher.progressMinimum(), m_watcher.progressMaximum());
    m_progress->setValue(m_watcher.progressValue());
//    if (m_watcher.progressMinimum() == 0 && m_watcher.progressMaximum() == 0)
//        m_progress->startAnimation();
}

void FutureProgress::setFinished()
{
//    m_progress->stopAnimation();
    setToolTip(m_watcher.future().progressText());
    if (m_watcher.future().isCanceled()) {
        m_progress->setError(true);
//        m_progress->execGlowOut(true);
    } else {
        m_progress->setError(false);
//        m_progress->execGlowOut(false);
    }
//    m_progress->showToolTip();
    emit finished();
}

void FutureProgress::setProgressRange(int min, int max)
{
    m_progress->setRange(min, max);
    if (min != 0 || max != 0) {
//        m_progress->setUsingAnimation(false);
    } else {
//        m_progress->setUsingAnimation(true);
        if (m_watcher.future().isRunning()) {
            //m_progress->startAnimation();
        }
    }
}

void FutureProgress::setProgressValue(int val)
{
    m_progress->setValue(val);
}

void FutureProgress::setProgressText(const QString &text)
{
    setToolTip(text);
}

void FutureProgress::setFuture(const QFuture<void> &future)
{
    m_watcher.setFuture(future);
}

QFuture<void> FutureProgress::future() const
{
    return m_watcher.future();
}

void FutureProgress::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked();
    QWidget::mousePressEvent(event);
}

bool FutureProgress::hasError() const
{
    return m_progress->hasError();
}
