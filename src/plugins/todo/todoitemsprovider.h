/**************************************************************************
**
** Copyright (C) 2015 Dmitry Savchenko
** Copyright (C) 2015 Vasiliy Sorokin
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

#ifndef TODOITEMSPROVIDER_H
#define TODOITEMSPROVIDER_H

#include "todoitem.h"
#include "settings.h"

#include <projectexplorer/project.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QHash>
#include <QList>

namespace Todo {
namespace Internal {

class TodoItemsModel;
class TodoItemsScanner;

class TodoItemsProvider : public QObject
{
    Q_OBJECT

public:
    explicit TodoItemsProvider(Settings settings, QObject *parent = 0);
    TodoItemsModel *todoItemsModel();

public slots:
    void settingsChanged(const Settings &newSettings);
    void projectSettingsChanged(ProjectExplorer::Project *project);

signals:
    void itemsUpdated();

private:
    Settings m_settings;
    TodoItemsModel *m_itemsModel;

    // All to-do items are stored here regardless current scanning scope
    QHash<QString, QList<TodoItem> > m_itemsHash;

    // This list contains only those to-do items that are within current scanning scope
    QList<TodoItem> m_itemsList;

    QList<TodoItemsScanner *> m_scanners;

    ProjectExplorer::Project *m_startupProject;
    Core::IEditor* m_currentEditor;

    bool m_shouldUpdateList;

    void setupItemsModel();
    void setupStartupProjectBinding();
    void setupCurrentEditorBinding();
    void setupUpdateListTimer();
    void updateList();
    void createScanners();
    void setItemsListWithinStartupProject();

private slots:
    void itemsFetched(const QString &fileName, const QList<TodoItem> &items);
    void startupProjectChanged(ProjectExplorer::Project *project);
    void projectsFilesChanged();
    void currentEditorChanged(Core::IEditor *editor);
    void updateListTimeoutElapsed();
};

}
}

#endif // TODOITEMSPROVIDER_H
