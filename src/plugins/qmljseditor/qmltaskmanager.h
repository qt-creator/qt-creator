// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QmlTaskManager();

    void extensionsInitialized();

    void updateMessages();
    void updateSemanticMessagesNow();
    void documentsRemoved(const Utils::FilePaths &path);

private:
    void displayResults(int begin, int end);
    void displayAllResults();
    void updateMessagesNow(bool updateSemantic = false);

    void insertTask(const ProjectExplorer::Task &task);
    void removeTasksForFile(const Utils::FilePath &fileName);
    void removeAllTasks(bool clearSemantic);

private:
    class FileErrorMessages
    {
    public:
        Utils::FilePath fileName;
        ProjectExplorer::Tasks tasks;
    };
    static void collectMessages(QPromise<FileErrorMessages> &promise,
                                QmlJS::Snapshot snapshot,
                                const QList<QmlJS::ModelManagerInterface::ProjectInfo> &projectInfos,
                                QmlJS::ViewerContext vContext,
                                bool updateSemantic);

private:
    QHash<Utils::FilePath, ProjectExplorer::Tasks> m_docsWithTasks;
    QFutureWatcher<FileErrorMessages> m_messageCollector;
    QTimer m_updateDelay;
    bool m_updatingSemantic = false;
};

} // Internal
} // QmlJSEditor
