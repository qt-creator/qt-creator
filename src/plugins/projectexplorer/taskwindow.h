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
    TaskWindow();
    virtual ~TaskWindow();

    void delayedInitialization();

    int taskCount(Core::Id category = Core::Id()) const;
    int warningTaskCount(Core::Id category = Core::Id()) const;
    int errorTaskCount(Core::Id category = Core::Id()) const;

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
    void addCategory(Core::Id categoryId, const QString &displayName, bool visible);
    void addTask(const ProjectExplorer::Task &task);
    void removeTask(const ProjectExplorer::Task &task);
    void updatedTaskFileName(unsigned int id, const QString &fileName);
    void updatedTaskLineNumber(unsigned int id, int line);
    void showTask(unsigned int id);
    void openTask(unsigned int id);
    void clearTasks(Core::Id categoryId);
    void setCategoryVisibility(Core::Id categoryId, bool visible);
    void currentChanged(const QModelIndex &index);
    void saveSettings();
    void loadSettings();

    void triggerDefaultHandler(const QModelIndex &index);
    void actionTriggered();
    void setShowWarnings(bool);
    void updateCategoriesMenu();

private:
    int sizeHintForColumn(int column) const;

    TaskWindowPrivate *d;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TASKWINDOW_H
