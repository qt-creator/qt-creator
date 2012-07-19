/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PROGRESSMANAGER_P_H
#define PROGRESSMANAGER_P_H

#include "progressmanager.h"

#include <QPointer>
#include <QList>
#include <QFutureWatcher>

namespace Core {
namespace Internal {

class ProgressView;

class ProgressManagerPrivate : public Core::ProgressManager
{
    Q_OBJECT
public:
    ProgressManagerPrivate(QObject *parent = 0);
    ~ProgressManagerPrivate();
    void init();
    void cleanup();

    FutureProgress *addTask(const QFuture<void> &future, const QString &title, const QString &type,
                            ProgressFlags flags);

    void setApplicationLabel(const QString &text);
    QWidget *progressView();

public slots:
    void cancelTasks(const QString &type);

private slots:
    void taskFinished();
    void cancelAllRunningTasks();
    void setApplicationProgressRange(int min, int max);
    void setApplicationProgressValue(int value);
    void setApplicationProgressVisible(bool visible);
    void disconnectApplicationTask();

private:
    QPointer<ProgressView> m_progressView;
    QMap<QFutureWatcher<void> *, QString> m_runningTasks;
    QFutureWatcher<void> *m_applicationTask;
};

} // namespace Internal
} // namespace Core

#endif // PROGRESSMANAGER_P_H
