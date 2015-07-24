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

#ifndef TASKHUB_H
#define TASKHUB_H

#include "task.h"

#include <QObject>
#include <QIcon>

namespace ProjectExplorer {

class ProjectExplorerPlugin;

class PROJECTEXPLORER_EXPORT TaskHub : public QObject
{
    Q_OBJECT
public:
    static QObject *instance();

    // Convenience overload
    static void addTask(Task::TaskType type, const QString &description,
                        Core::Id category,
                        const Utils::FileName &file = Utils::FileName(),
                        int line = -1);

public slots:
    static void addTask(ProjectExplorer::Task task);
    static void clearTasks(Core::Id categoryId = Core::Id());
    static void removeTask(const ProjectExplorer::Task &task);

public:
    static void addCategory(Core::Id categoryId, const QString &displayName, bool visible = true);
    static void updateTaskFileName(unsigned int id, const QString &fileName);
    static void updateTaskLineNumber(unsigned int id, int line);
    static void taskMarkClicked(unsigned int id);
    static void showTaskInEditor(unsigned int id);
    static void setCategoryVisibility(Core::Id categoryId, bool visible);

    static void requestPopup();

signals:
    void categoryAdded(Core::Id categoryId, const QString &displayName, bool visible);
    void taskAdded(const ProjectExplorer::Task &task);
    void taskRemoved(const ProjectExplorer::Task &task);
    void tasksCleared(Core::Id categoryId);
    void taskFileNameUpdated(unsigned int id, const QString &fileName);
    void taskLineNumberUpdated(unsigned int id, int line);
    void categoryVisibilityChanged(Core::Id categoryId, bool visible);
    void popupRequested(int);
    void showTask(unsigned int id);
    void openTask(unsigned int id);

private:
    TaskHub();
    ~TaskHub();

    const QIcon m_errorIcon;
    const QIcon m_warningIcon;

    static QVector<Core::Id> m_registeredCategories;

    friend class ProjectExplorerPlugin;
};

} // namespace ProjectExplorer
#endif // TASKHUB_H
