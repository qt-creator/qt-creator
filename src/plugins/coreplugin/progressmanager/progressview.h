/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#ifndef PROGRESSVIEW_H
#define PROGRESSVIEW_H

#include "progressmanagerinterface.h"

#include <QtCore/QFuture>
#include <QtGui/QWidget>
#include <QtGui/QIcon>
#include <QtGui/QVBoxLayout>

namespace Core {

class FutureProgress;

namespace Internal {

class ProgressView : public QWidget
{
    Q_OBJECT

public:
    ProgressView(QWidget *parent = 0);
    ~ProgressView();

    /** The returned FutureProgress instance is guaranteed to live till next main loop event processing (deleteLater). */
    FutureProgress *addTask(const QFuture<void> &future,
                            const QString &title,
                            const QString &type,
                            ProgressManagerInterface::PersistentType persistency);

private slots:
    void slotFinished();

private:
    void removeOldTasks(const QString &type, bool keepOne = false);
    void removeOneOldTask();
    void removeTask(FutureProgress *task);
    void deleteTask(FutureProgress *task);

    QVBoxLayout *m_layout;
    QList<FutureProgress *> m_taskList;
    QHash<FutureProgress *, QString> m_type;
    QHash<FutureProgress *, bool> m_keep;
};

} // namespace Internal
} // namespace Core

#endif // PROGRESSVIEW_H
