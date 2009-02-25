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

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>
#include <texteditor/plaintexteditor.h>

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtCore/QBasicTimer>
#include <QtGui/QAbstractScrollArea>
#include <QtGui/QToolButton>
#include <QtGui/QPlainTextEdit>

QT_BEGIN_NAMESPACE
class QTabWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {

class RunControl;

namespace Internal {

class OutputWindow;

class OutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    OutputPane();
    ~OutputPane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets(void) const;
    QString name() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool);
    bool canFocus();
    bool hasFocus();
    void setFocus();

    void appendOutput(const QString &out);

    // ApplicationOutputspecifics
    void createNewOutputWindow(RunControl *rc);
    void appendOutput(RunControl *rc, const QString &out);
    void appendOutputInline(RunControl *rc, const QString &out);
    void showTabFor(RunControl *rc);
    
public slots:
    void projectRemoved();
    void coreAboutToClose();

private slots:
    void insertLine();
    void reRunRunControl();
    void stopRunControl();
    void closeTab(int index);
    void tabChanged(int);
    void runControlStarted();
    void runControlFinished();

private:
    RunControl *runControlForTab(int index) const;

    QWidget *m_mainWidget;
    QTabWidget *m_tabWidget;
    QHash<RunControl *, OutputWindow *> m_outputWindows;
    QAction *m_stopAction;
//    QToolButton *m_insertLineButton;
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
};


class OutputWindow : public QPlainTextEdit
{
    Q_OBJECT

public:
    OutputWindow(QWidget *parent = 0);
    ~OutputWindow();

    void appendOutput(const QString &out);
    void appendOutputInline(const QString &out);
    void insertLine();
};

#if 0
class OutputWindow
  : public QAbstractScrollArea
{
    Q_OBJECT

    int max_lines;
    bool same_height;
    int width_used;
    bool block_scroll;
    QStringList lines;
    QBasicTimer autoscroll_timer;
    int autoscroll;
    QPoint lastMouseMove;


    struct Selection {
        Selection():line(0), pos(0){}
        int line;
        int pos;

        bool operator==(const Selection &other) const
            { return line == other.line && pos == other.pos; }
        bool operator!=(const Selection &other) const
            { return !(*this == other); }
        bool operator<(const Selection &other) const
            { return line < other.line || (line == other.line && pos < other.pos); }
        bool operator>=(const Selection &other) const
        { return !(*this < other); }
        bool operator<=(const Selection &other) const
            { return line < other.line || (line == other.line && pos == other.pos); }
        bool operator>(const Selection &other) const
        { return !(*this <= other); }
    };

    Selection selection_start, selection_end;
    void changed();
    bool getCursorPos(int *lineNumber, int *position, const QPoint &pos);

public:
    OutputWindow(QWidget *parent = 0);
    ~OutputWindow();

    void setNumberOfLines(int max);
    int numberOfLines() const;

    bool hasSelectedText() const;
    void clearSelection();

    QString selectedText() const;

    void appendOutput(const QString &out);
    void insertLine() {
        appendOutput(QChar(QChar::ParagraphSeparator));
    }


public slots:
    void clear();
    void copy();
    void selectAll();

signals:
    void showPage();

protected:
    void scrollContentsBy(int dx, int dy);
    void keyPressEvent(QKeyEvent *e);
    void paintEvent(QPaintEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);
    void timerEvent(QTimerEvent *e);
    void contextMenuEvent(QContextMenuEvent * e);
};
#endif // 0
} // namespace Internal
} // namespace ProjectExplorer

#endif // OUTPUTWINDOW_H
