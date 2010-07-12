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

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>
#include <projectexplorer/outputformatter.h>

#include <QtCore/QObject>
#include <QtCore/QHash>
#include <QtGui/QShowEvent>
#include <QtGui/QPlainTextEdit>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QToolButton;
class QAction;
QT_END_NAMESPACE

namespace Core {
    class BaseContext;
}

namespace ProjectExplorer {

class RunControl;

namespace Constants {
    const char * const C_APP_OUTPUT = "Application Output";
}

namespace Internal {

class OutputWindow;

class OutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    OutputPane();
    ~OutputPane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets() const;
    QString name() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool);
    bool canFocus();
    bool hasFocus();
    void setFocus();

    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();
    bool canNavigate();

    void showTabFor(RunControl *rc);

public slots:
    // ApplicationOutputspecifics
    void createNewOutputWindow(RunControl *rc);
    void projectRemoved();
    void coreAboutToClose();

    void appendApplicationOutput(RunControl *rc, const QString &out,
                                 bool onStdErr);
    void appendApplicationOutputInline(RunControl *rc, const QString &out,
                                       bool onStdErr);
    void appendMessage(RunControl *rc, const QString &out, bool isError);

private slots:
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
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
};


class OutputWindow : public QPlainTextEdit
{
    Q_OBJECT

public:
    OutputWindow(QWidget *parent = 0);
    ~OutputWindow();

    OutputFormatter* formatter() const;
    void setFormatter(OutputFormatter *formatter);

    void appendApplicationOutput(const QString &out, bool onStdErr);
    void appendApplicationOutputInline(const QString &out, bool onStdErr);
    void appendMessage(const QString &out, bool isError);
    /// appends a \p text using \p format without using formater
    void appendText(const QString &text, const QTextCharFormat &format, int maxLineCount);

    void grayOutOldContent();

    void showEvent(QShowEvent *);

    void clear()
    {
        m_enforceNewline = false;
        QPlainTextEdit::clear();
    }

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *e);

    bool isScrollbarAtBottom() const;
    void scrollToBottom();

private:
    void enableUndoRedo();
    QString doNewlineEnfocement(const QString &out);

private:
    Core::BaseContext *m_outputWindowContext;
    OutputFormatter *m_formatter;

    bool m_enforceNewline;
    bool m_scrollToBottom;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // OUTPUTWINDOW_H
