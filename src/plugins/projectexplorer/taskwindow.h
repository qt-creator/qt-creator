/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef TASKWINDOW_H
#define TASKWINDOW_H

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

// Show build issues (warnings or errors) and open the editor on click.
class TaskWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    TaskWindow(ProjectExplorer::TaskHub *taskHub);
    virtual ~TaskWindow();

    int taskCount() const;
    int errorTaskCount() const;

    // IOutputPane
    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets() const;

    QString displayName() const { return tr("Build Issues"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);

    bool canFocus();
    bool hasFocus();
    void setFocus();

    bool canNavigate();
    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();

signals:
    void tasksChanged();
    void tasksCleared();

private slots:
    void addCategory(const QString &categoryId, const QString &displayName);
    void addTask(const ProjectExplorer::Task &task);
    void removeTask(const ProjectExplorer::Task &task);
    void clearTasks(const QString &categoryId);

    void triggerDefaultHandler(const QModelIndex &index);
    void showContextMenu(const QPoint &position);
    void contextMenuEntryTriggered(QAction *);
    void setShowWarnings(bool);
    void updateCategoriesMenu();
    void filterMenuTriggered(QAction *action);

private:
    void cleanContextMenu();
    int sizeHintForColumn(int column) const;

    TaskWindowPrivate *d;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TASKWINDOW_H
