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

#pragma once

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

    void updateMessages();
    void updateSemanticMessagesNow();
    void documentsRemoved(const QStringList &path);

private:
    void displayResults(int begin, int end);
    void displayAllResults();
    void updateMessagesNow(bool updateSemantic = false);

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
