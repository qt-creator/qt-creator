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

#ifndef QMLTASKMANAGER_H
#define QMLTASKMANAGER_H

#include <projectexplorer/task.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsmodelmanagerinterface.h>

#include <QObject>
#include <QList>
#include <QHash>
#include <QString>
#include <QFutureWatcher>
#include <QTimer>

namespace QmlJSEditor {
namespace Internal {

class QmlTaskManager : public QObject
{
    Q_OBJECT
public:
    QmlTaskManager(QObject *parent = 0);

    void extensionsInitialized();

public slots:
    void updateMessages();
    void updateSemanticMessagesNow();
    void documentsRemoved(const QStringList &path);

private slots:
    void displayResults(int begin, int end);
    void displayAllResults();
    void updateMessagesNow(bool updateSemantic = false);

private:
    void insertTask(const ProjectExplorer::Task &task);
    void removeTasksForFile(const QString &fileName);
    void removeAllTasks(bool clearSemantic);

private:
    class FileErrorMessages
    {
    public:
        QString fileName;
        QList<ProjectExplorer::Task> tasks;
    };
    static void collectMessages(QFutureInterface<FileErrorMessages> &future,
                                QmlJS::Snapshot snapshot,
                                QList<QmlJS::ModelManagerInterface::ProjectInfo> projectInfos,
                                QmlJS::ViewerContext vContext,
                                bool updateSemantic);

private:
    QHash<QString, QList<ProjectExplorer::Task> > m_docsWithTasks;
    QFutureWatcher<FileErrorMessages> m_messageCollector;
    QTimer m_updateDelay;
    bool m_updatingSemantic;
};

} // Internal
} // QmlJSEditor

#endif // QMLTASKMANAGER_H
