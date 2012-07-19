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

#ifndef TASKWINDOW_H
#define TASKWINDOW_H

#include <coreplugin/id.h>
#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QAction;
class QModelIndex;
class QPoint;
QT_END_NAMESPACE

namespace ProjectExplorer {
class TaskHub;
class Task;

namespace Internal {
class TaskWindowPrivate;

// Show issues (warnings or errors) and open the editor on click.
class TaskWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    TaskWindow(ProjectExplorer::TaskHub *taskHub);
    virtual ~TaskWindow();

    int taskCount(const Core::Id &category = Core::Id()) const;
    int warningTaskCount(const Core::Id &category = Core::Id()) const;
    int errorTaskCount(const Core::Id &category = Core::Id()) const;

    // IOutputPane
    QWidget *outputWidget(QWidget *);
    QList<QWidget *> toolBarWidgets() const;

    QString displayName() const { return tr("Issues"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);

    bool canFocus() const;
    bool hasFocus() const;
    void setFocus();

    bool canNavigate() const;
    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();

signals:
    void tasksChanged();
    void tasksCleared();

private slots:
    void addCategory(const Core::Id &categoryId, const QString &displayName, bool visible);
    void addTask(const ProjectExplorer::Task &task);
    void removeTask(const ProjectExplorer::Task &task);
    void updatedTaskFileName(unsigned int id, const QString &fileName);
    void updatedTaskLineNumber(unsigned int id, int line);
    void showTask(unsigned int id);
    void openTask(unsigned int id);
    void clearTasks(const Core::Id &categoryId);
    void setCategoryVisibility(const Core::Id &categoryId, bool visible);

    void triggerDefaultHandler(const QModelIndex &index);
    void showContextMenu(const QPoint &position);
    void contextMenuEntryTriggered(QAction *);
    void setShowWarnings(bool);
    void updateCategoriesMenu();
    void filterCategoryTriggered(QAction *action);

private:
    void cleanContextMenu();
    int sizeHintForColumn(int column) const;

    TaskWindowPrivate *d;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TASKWINDOW_H
