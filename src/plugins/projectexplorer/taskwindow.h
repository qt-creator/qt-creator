/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef TASKWINDOW_H
#define TASKWINDOW_H

#include "buildparserinterface.h"

#include <coreplugin/ioutputpane.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

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

    Core::ICore *m_coreIFace;
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
