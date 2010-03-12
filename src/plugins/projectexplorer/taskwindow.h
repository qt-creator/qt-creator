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

#include "projectexplorer_export.h"

#include <coreplugin/icontext.h>
#include <coreplugin/ioutputpane.h>

#include <QtCore/QModelIndex>
#include <QtGui/QAction>
#include <QtGui/QToolButton>

namespace ProjectExplorer {

namespace Internal {

class TaskModel;
class TaskFilterModel;
class TaskView;
class TaskWindowContext;

} // namespace Internal

struct Task {
    enum TaskType {
        Unknown,
        Error,
        Warning
    };

    Task() : type(Unknown), line(-1)
    { }
    Task(TaskType type_, const QString &description_,
         const QString &file_, int line_, const QString &category_) :
        type(type_), description(description_), file(file_), line(line_), category(category_)
    { }
    Task(const Task &source) :
        type(source.type), description(source.description), file(source.file),
        line(source.line), category(source.category)
    { }
    ~Task()
    { }

    TaskType type;
    QString description;
    QString file;
    int line;
    QString category;
};

class PROJECTEXPLORER_EXPORT TaskWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    TaskWindow();
    ~TaskWindow();

    void addCategory(const QString &categoryId, const QString &displayName);

    void addTask(const Task &task);
    void clearTasks(const QString &categoryId = QString());

    int taskCount(const QString &categoryId = QString()) const;
    int errorTaskCount(const QString &categoryId = QString()) const;


    // IOutputPane
    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets() const;

    QString name() const { return tr("Build Issues"); }
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

private slots:
    void showTaskInFile(const QModelIndex &index);
    void copy();
    void setShowWarnings(bool);
    void updateCategoriesMenu();
    void filterCategoryTriggered(QAction *action);

private:
    void updateActions();
    int sizeHintForColumn(int column) const;

    Internal::TaskModel *m_model;
    Internal::TaskFilterModel *m_filter;
    Internal::TaskView *m_listview;
    Internal::TaskWindowContext *m_taskWindowContext;
    QAction *m_copyAction;
    QToolButton *m_filterWarningsButton;
    QToolButton *m_categoriesButton;
    QMenu *m_categoriesMenu;
};

bool operator==(const Task &t1, const Task &t2);
uint qHash(const Task &task);

} //namespace ProjectExplorer

#endif // TASKWINDOW_H
