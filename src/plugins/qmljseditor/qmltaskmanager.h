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
class QmlJSTextEditorWidget;
}

namespace ProjectExplorer {
class TaskHub;
} // namespace ProjectExplorer

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
    void documentsRemoved(const QStringList path);

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
                                QStringList importPaths,
                                bool updateSemantic);

private:
    ProjectExplorer::TaskHub *m_taskHub;
    QHash<QString, QList<ProjectExplorer::Task> > m_docsWithTasks;
    QFutureWatcher<FileErrorMessages> m_messageCollector;
    QTimer m_updateDelay;
    bool m_updatingSemantic;
};

} // Internal
} // QmlJSEditor

#endif // QMLTASKMANAGER_H
