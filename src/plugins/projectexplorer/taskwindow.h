/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef TASKWINDOW_H
#define TASKWINDOW_H

#include "buildparserinterface.h"

#include <coreplugin/ioutputpane.h>
#include <coreplugin/icontext.h>

#include <QtGui/QTreeWidget>
#include <QtGui/QStyledItemDelegate>
#include <QtGui/QListView>
#include <QtGui/QToolButton>

namespace ProjectExplorer {
namespace Internal {

class TaskModel;
class TaskView;
class TaskWindowContext;

class TaskWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    TaskWindow();
    ~TaskWindow();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets(void) const;

    QString name() const { return tr("Build Issues"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);

    void addItem(BuildParserInterface::PatternType type,
        const QString &description, const QString &file, int line);

    int numberOfTasks() const;
    int numberOfErrors() const;

    void gotoFirstError();
    bool canFocus();
    bool hasFocus();
    void setFocus();

signals:
    void tasksChanged();

private slots:
    void showTaskInFile(const QModelIndex &index);
    void copy();

private:
    int sizeHintForColumn(int column) const;

    int m_errorCount;
    int m_currentTask;

    TaskModel *m_model;
    TaskView *m_listview;
    TaskWindowContext *m_taskWindowContext;
    QAction *m_copyAction;
};

class TaskView : public QListView
{
public:
    TaskView(QWidget *parent = 0);
    ~TaskView();
    void resizeEvent(QResizeEvent *e);
    void keyPressEvent(QKeyEvent *e);
};

class TaskDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    TaskDelegate(QObject * parent = 0);
    ~TaskDelegate();
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;

    // TaskView uses this method if the size of the taskview changes
    void emitSizeHintChanged(const QModelIndex &index);

public slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

private:
    void generateGradientPixmap(int width, int height, QColor color, bool selected) const;
};

class TaskWindowContext : public Core::IContext
{
public:
    TaskWindowContext(QWidget *widget);
    virtual QList<int> context() const;
    virtual QWidget *widget();
private:
    QWidget *m_taskList;
    QList<int> m_context;
};

} //namespace Internal
} //namespace ProjectExplorer

#endif // TASKWINDOW_H
