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

#include "futureprogress.h"
#include "progressbar.h"

#include <utils/stylehelper.h>

#include <QCoreApplication>
#include <QFutureWatcher>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPainter>
#include <QSequentialAnimationGroup>
#include <QTimer>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>

const int notificationTimeout = 8000;
const int shortNotificationTimeout = 1000;

namespace Core {

class FutureProgressPrivate : public QObject
{
    Q_OBJECT

public slots:
    void fadeAway();

public:
    explicit FutureProgressPrivate(FutureProgress *q);

    void tryToFadeAway();

    QFutureWatcher<void> m_watcher;
    Internal::ProgressBar *m_progress;
    QWidget *m_widget;
    QHBoxLayout *m_widgetLayout;
    QWidget *m_statusBarWidget;
    QString m_type;
    FutureProgress::KeepOnFinishType m_keep;
    bool m_waitingForUserInteraction;
    FutureProgress *m_q;
    bool m_fadeStarting;
    bool m_isFading;
};

FutureProgressPrivate::FutureProgressPrivate(FutureProgress *q) :
    m_progress(new Internal::ProgressBar), m_widget(0), m_widgetLayout(new QHBoxLayout),
    m_statusBarWidget(0),
    m_keep(FutureProgress::HideOnFinish), m_waitingForUserInteraction(false),
    m_q(q), m_fadeStarting(false), m_isFading(false)
{
}

/*!
    \mainclass
    \class Core::FutureProgress
    \brief The FutureProgress class is used to adapt the appearance of
    progress indicators that were created through the ProgressManager class.

    Use the instance of this class that was returned by
    ProgressManager::addTask() to define a widget that
    should be shown below the progress bar, or to change the
    progress title.
    Also use it to react on the event that the user clicks on
    the progress indicator (which can be used to e.g. open a more detailed
    view, or the results of the task).
*/

/*!
    \fn void FutureProgress::clicked()
    Connect to this signal to get informed when the user clicks on the
    progress indicator.
*/

/*!
    \fn void FutureProgress::canceled()
    Connect to this signal to get informed when the operation is canceled.
*/

/*!
    \fn void FutureProgress::finished()
    Another way to get informed when the task has finished.
*/

/*!
    \fn QWidget FutureProgress::widget() const
    Returns the custom widget that is shown below the progress indicator.
*/

/*!
    \fn FutureProgress::FutureProgress(QWidget *parent)
    \internal
*/
FutureProgress::FutureProgress(QWidget *parent) :
    QWidget(parent), d(new FutureProgressPrivate(this))
{
    QVBoxLayout *layout = new QVBoxLayout;
    setLayout(layout);
    layout->addWidget(d->m_progress);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addLayout(d->m_widgetLayout);
    d->m_widgetLayout->setContentsMargins(7, 0, 7, 2);
    d->m_widgetLayout->setSpacing(0);

    connect(&d->m_watcher, SIGNAL(started()), this, SLOT(setStarted()));
    connect(&d->m_watcher, SIGNAL(finished()), this, SLOT(setFinished()));
    connect(&d->m_watcher, SIGNAL(canceled()), this, SIGNAL(canceled()));
    connect(&d->m_watcher, SIGNAL(progressRangeChanged(int,int)), this, SLOT(setProgressRange(int,int)));
    connect(&d->m_watcher, SIGNAL(progressValueChanged(int)), this, SLOT(setProgressValue(int)));
    connect(&d->m_watcher, SIGNAL(progressTextChanged(QString)),
            this, SLOT(setProgressText(QString)));
    connect(d->m_progress, SIGNAL(clicked()), this, SLOT(cancel()));
}

/*!
    \fn FutureProgress::~FutureProgress()
    \internal
*/
FutureProgress::~FutureProgress()
{
    delete d->m_widget;
    delete d;
}

/*!
    \fn void FutureProgress::setWidget(QWidget *widget)
    Sets the \a widget to show below the progress bar.
    This will be destroyed when the progress indicator is destroyed.
    Default is to show no widget below the progress indicator.
*/
void FutureProgress::setWidget(QWidget *widget)
{
    delete d->m_widget;
    QSizePolicy sp = widget->sizePolicy();
    sp.setHorizontalPolicy(QSizePolicy::Ignored);
    widget->setSizePolicy(sp);
    d->m_widget = widget;
    if (d->m_widget)
        d->m_widgetLayout->addWidget(d->m_widget);
}

/*!
    \fn void FutureProgress::setTitle(const QString &title)
    Changes the \a title of the progress indicator.
*/
void FutureProgress::setTitle(const QString &title)
{
    d->m_progress->setTitle(title);
}

/*!
    \fn QString FutureProgress::title() const
    Returns the title of the progress indicator.
*/
QString FutureProgress::title() const
{
    return d->m_progress->title();
}

void FutureProgress::cancel()
{
    d->m_watcher.future().cancel();
}

void FutureProgress::updateToolTip(const QString &text)
{
    setToolTip(QLatin1String("<b>") + title() + QLatin1String("</b><br>") + text);
}

void FutureProgress::setStarted()
{
    d->m_progress->reset();
    d->m_progress->setError(false);
    d->m_progress->setRange(d->m_watcher.progressMinimum(), d->m_watcher.progressMaximum());
    d->m_progress->setValue(d->m_watcher.progressValue());
}


bool FutureProgress::eventFilter(QObject *, QEvent *e)
{
    if (d->m_keep != KeepOnFinish && d->m_waitingForUserInteraction
            && (e->type() == QEvent::MouseMove || e->type() == QEvent::KeyPress)) {
        qApp->removeEventFilter(this);
        QTimer::singleShot(notificationTimeout, d, SLOT(fadeAway()));
    }
    return false;
}

void FutureProgress::setFinished()
{
    updateToolTip(d->m_watcher.future().progressText());

    d->m_progress->setFinished(true);

    if (d->m_watcher.future().isCanceled()) {
        d->m_progress->setError(true);
        emit hasErrorChanged();
    } else {
        d->m_progress->setError(false);
    }
    emit finished();
    d->tryToFadeAway();
}

void FutureProgressPrivate::tryToFadeAway()
{
    if (m_fadeStarting)
        return;
    if (m_keep == FutureProgress::KeepOnFinishTillUserInteraction
            || (m_keep == FutureProgress::HideOnFinish && m_progress->hasError())) {
        m_waitingForUserInteraction = true;
        //eventfilter is needed to get user interaction
        //events to start QTimer::singleShot later
        qApp->installEventFilter(m_q);
        m_fadeStarting = true;
    } else if (m_keep == FutureProgress::HideOnFinish) {
        QTimer::singleShot(shortNotificationTimeout, this, SLOT(fadeAway()));
        m_fadeStarting = true;
    }
}


void FutureProgress::setProgressRange(int min, int max)
{
    d->m_progress->setRange(min, max);
}

void FutureProgress::setProgressValue(int val)
{
    d->m_progress->setValue(val);
}

void FutureProgress::setProgressText(const QString &text)
{
    updateToolTip(text);
}

/*!
    \fn void FutureProgress::setFuture(const QFuture<void> &future)
    \internal
*/
void FutureProgress::setFuture(const QFuture<void> &future)
{
    d->m_watcher.setFuture(future);
}

/*!
    \fn QFuture<void> FutureProgress::future() const
    Returns a QFuture object that represents this running task.
*/
QFuture<void> FutureProgress::future() const
{
    return d->m_watcher.future();
}

/*!
    \fn void FutureProgress::mousePressEvent(QMouseEvent *event)
    \internal
*/
void FutureProgress::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
        emit clicked();
    QWidget::mousePressEvent(event);
}

void FutureProgress::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    QLinearGradient grad = Utils::StyleHelper::statusBarGradient(rect());
    p.fillRect(rect(), grad);
}

/*!
    \fn bool FutureProgress::hasError() const
    Returns the error state of this progress indicator.
*/
bool FutureProgress::hasError() const
{
    return d->m_progress->hasError();
}

void FutureProgress::setType(const QString &type)
{
    d->m_type = type;
}

QString FutureProgress::type() const
{
    return d->m_type;
}

void FutureProgress::setKeepOnFinish(KeepOnFinishType keepType)
{
    if (d->m_keep == keepType)
        return;
    d->m_keep = keepType;

    //if it is not finished tryToFadeAway is called by setFinished at the end
    if (d->m_watcher.isFinished())
        d->tryToFadeAway();
}

bool FutureProgress::keepOnFinish() const
{
    return d->m_keep;
}

QWidget *FutureProgress::widget() const
{
    return d->m_widget;
}

void FutureProgress::setStatusBarWidget(QWidget *widget)
{
    if (widget == d->m_statusBarWidget)
        return;
    delete d->m_statusBarWidget;
    d->m_statusBarWidget = widget;
    emit statusBarWidgetChanged();
}

QWidget *FutureProgress::statusBarWidget() const
{
    return d->m_statusBarWidget;
}

bool FutureProgress::isFading() const
{
    return d->m_isFading;
}

QSize FutureProgress::sizeHint() const
{
    return QSize(100, minimumHeight());
}

void FutureProgressPrivate::fadeAway()
{
    m_isFading = true;

    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect;
    opacityEffect->setOpacity(1.);
    m_q->setGraphicsEffect(opacityEffect);

    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(this);
    QPropertyAnimation *animation = new QPropertyAnimation(opacityEffect, "opacity");
    animation->setDuration(Utils::StyleHelper::progressFadeAnimationDuration);
    animation->setEndValue(0.);
    group->addAnimation(animation);
    animation = new QPropertyAnimation(m_q, "maximumHeight");
    animation->setDuration(120);
    animation->setEasingCurve(QEasingCurve::InCurve);
    animation->setStartValue(m_q->sizeHint().height());
    animation->setEndValue(0.0);
    group->addAnimation(animation);

    connect(group, SIGNAL(finished()), m_q, SIGNAL(removeMe()));
    group->start(QAbstractAnimation::DeleteWhenStopped);
    emit m_q->fadeStarted();
}

} // namespace Core

#include "futureprogress.moc"
