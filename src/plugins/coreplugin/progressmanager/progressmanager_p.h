/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PROGRESSMANAGER_P_H
#define PROGRESSMANAGER_P_H

#include "progressmanager.h"

#include <QFutureWatcher>
#include <QList>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QToolButton>

namespace Core {

class StatusBarWidget;

namespace Internal {

class ProgressBar;
class ProgressView;

class ProgressManagerPrivate : public Core::ProgressManager
{
    Q_OBJECT
public:
    ProgressManagerPrivate();
    ~ProgressManagerPrivate();
    void init();
    void cleanup();

    FutureProgress *doAddTask(const QFuture<void> &future, const QString &title, Id type,
                            ProgressFlags flags);

    void doSetApplicationLabel(const QString &text);
    ProgressView *progressView();

public slots:
    void doCancelTasks(Core::Id type);

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private slots:
    void taskFinished();
    void cancelAllRunningTasks();
    void setApplicationProgressRange(int min, int max);
    void setApplicationProgressValue(int value);
    void setApplicationProgressVisible(bool visible);
    void disconnectApplicationTask();
    void updateSummaryProgressBar();
    void fadeAwaySummaryProgress();
    void summaryProgressFinishedFading();
    void progressDetailsToggled(bool checked);
    void updateVisibility();
    void updateVisibilityWithDelay();
    void updateStatusDetailsWidget();

    void slotRemoveTask();
private:
    void readSettings();
    void initInternal();
    void stopFadeOfSummaryProgress();

    bool hasError() const;
    bool isLastFading() const;

    void removeOldTasks(Id type, bool keepOne = false);
    void removeOneOldTask();
    void removeTask(FutureProgress *task);
    void deleteTask(FutureProgress *task);

    QPointer<ProgressView> m_progressView;
    QList<FutureProgress *> m_taskList;
    QMap<QFutureWatcher<void> *, Id> m_runningTasks;
    QFutureWatcher<void> *m_applicationTask;
    StatusBarWidget *m_statusBarWidgetContainer;
    QWidget *m_statusBarWidget;
    QWidget *m_summaryProgressWidget;
    QHBoxLayout *m_summaryProgressLayout;
    QWidget *m_currentStatusDetailsWidget;
    QPointer<FutureProgress> m_currentStatusDetailsProgress;
    ProgressBar *m_summaryProgressBar;
    QGraphicsOpacityEffect *m_opacityEffect;
    QPointer<QPropertyAnimation> m_opacityAnimation;
    bool m_progressViewPinned;
    bool m_hovered;
};

class ToggleButton : public QToolButton
{
    Q_OBJECT
public:
    ToggleButton(QWidget *parent);
    QSize sizeHint() const;
    void paintEvent(QPaintEvent *event);
};


class ProgressTimer : public QTimer
{
    Q_OBJECT

public:
    ProgressTimer(QObject *parent, const QFutureInterface<void> &futureInterface, int expectedSeconds);

private slots:
    void handleTimeout();

private:
    QFutureInterface<void> m_futureInterface;
    QFutureWatcher<void> m_futureWatcher;
    int m_expectedTime;
    int m_currentTime;
};

} // namespace Internal
} // namespace Core

#endif // PROGRESSMANAGER_P_H
