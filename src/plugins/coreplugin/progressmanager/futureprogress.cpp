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

#include <QFutureWatcher>
#include <QTimer>
#include <QCoreApplication>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

#include <QColor>
#include <QVBoxLayout>
#include <QMenu>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QPainter>
#include <QMouseEvent>
#include <utils/stylehelper.h>

const int notificationTimeout = 8000;
const int shortNotificationTimeout = 1000;

namespace Core {
namespace Internal {

class FadeWidgetHack : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity)
public:
    FadeWidgetHack(QWidget *parent):QWidget(parent), m_opacity(0){
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }
    void paintEvent(QPaintEvent *);

    void setOpacity(float o) { m_opacity = o; update(); }
    float opacity() const { return m_opacity; }

private:
    float m_opacity;
};

void FadeWidgetHack::paintEvent(QPaintEvent *)
{
    if (m_opacity == 0)
        return;

    QPainter p(this);
    p.setOpacity(m_opacity);
    if (m_opacity > 0)
        Utils::StyleHelper::verticalGradient(&p, rect(), rect());
}

} // namespace Internal

struct FutureProgressPrivate {
    explicit FutureProgressPrivate(FutureProgress *q);

    void tryToFadeAway();

    QFutureWatcher<void> m_watcher;
    Internal::ProgressBar *m_progress;
    QWidget *m_widget;
    QHBoxLayout *m_widgetLayout;
    QString m_type;
    FutureProgress::KeepOnFinishType m_keep;
    bool m_waitingForUserInteraction;
    Internal::FadeWidgetHack *m_faderWidget;
    FutureProgress *m_q;
    bool m_isFading;
};

FutureProgressPrivate::FutureProgressPrivate(FutureProgress *q) :
    m_progress(new Internal::ProgressBar), m_widget(0), m_widgetLayout(new QHBoxLayout),
    m_keep(FutureProgress::HideOnFinish), m_waitingForUserInteraction(false),
    m_faderWidget(new Internal::FadeWidgetHack(q)), m_q(q), m_isFading(false)
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
        QTimer::singleShot(notificationTimeout, this, SLOT(fadeAway()));
    }
    return false;
}

void FutureProgress::setFinished()
{
    updateToolTip(d->m_watcher.future().progressText());

    d->m_progress->setFinished(true);

    if (d->m_watcher.future().isCanceled())
        d->m_progress->setError(true);
    else
        d->m_progress->setError(false);
    emit finished();
    d->tryToFadeAway();
}

void FutureProgressPrivate::tryToFadeAway()
{
    if (m_isFading)
        return;
    if (m_keep == FutureProgress::KeepOnFinishTillUserInteraction
            || (m_keep == FutureProgress::HideOnFinish && m_progress->hasError())) {
        m_waitingForUserInteraction = true;
        //eventfilter is needed to get user interaction
        //events to start QTimer::singleShot later
        qApp->installEventFilter(m_q);
        m_isFading = true;
    } else if (m_keep == FutureProgress::HideOnFinish) {
        QTimer::singleShot(shortNotificationTimeout, m_q, SLOT(fadeAway()));
        m_isFading = true;
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

void FutureProgress::resizeEvent(QResizeEvent *)
{
    d->m_faderWidget->setGeometry(rect());
}

/*!
    \fn bool FutureProgress::hasError() const
    Returns the error state of this progress indicator.
*/
bool FutureProgress::hasError() const
{
    return d->m_progress->hasError();
}

void FutureProgress::fadeAway()
{
    d->m_faderWidget->raise();
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(this);
    QPropertyAnimation *animation = new QPropertyAnimation(d->m_faderWidget, "opacity");
    animation->setDuration(600);
    animation->setEndValue(1.0);
    group->addAnimation(animation);
    animation = new QPropertyAnimation(this, "maximumHeight");
    animation->setDuration(120);
    animation->setEasingCurve(QEasingCurve::InCurve);
    animation->setStartValue(sizeHint().height());
    animation->setEndValue(0.0);
    group->addAnimation(animation);
    group->start(QAbstractAnimation::DeleteWhenStopped);

    connect(group, SIGNAL(finished()), this, SIGNAL(removeMe()));
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

} // namespace Core

#include "futureprogress.moc"
