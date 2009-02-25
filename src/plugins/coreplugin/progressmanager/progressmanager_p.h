/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef PROGRESSMANAGER_P_H
#define PROGRESSMANAGER_P_H

#include "progressmanager.h"

#include <QtCore/QPointer>
#include <QtCore/QList>
#include <QtCore/QFutureWatcher>

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

    FutureProgress *addTask(const QFuture<void> &future, const QString &title, const QString &type, PersistentType persistency);

    QWidget *progressView();

public slots:
    void cancelTasks(const QString &type);

private slots:
    void taskFinished();
    void cancelAllRunningTasks();
private:
    QPointer<ProgressView> m_progressView;
    QMap<QFutureWatcher<void> *, QString> m_runningTasks;
};

} // namespace Internal
} // namespace Core

#endif // PROGRESSMANAGER_P_H
