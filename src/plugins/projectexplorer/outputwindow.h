/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include "outputformat.h"
#include <coreplugin/ioutputpane.h>

#include <QtGui/QPlainTextEdit>
#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QToolButton;
class QAction;
QT_END_NAMESPACE

namespace Core {
    class BaseContext;
}

namespace ProjectExplorer {
class OutputFormatter;
class RunControl;
class Project;

namespace Constants {
    const char * const C_APP_OUTPUT = "Application Output";
}

namespace Internal {

class OutputWindow;

struct OutputPanePrivate;

class OutputPane : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum CloseTabMode {
        CloseTabNoPrompt,
        CloseTabWithPrompt
    };

    OutputPane();
    virtual ~OutputPane();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets() const;
    QString displayName() const;
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

    void createNewOutputWindow(RunControl *rc);
    void showTabFor(RunControl *rc);

    bool aboutToClose() const;
    bool closeTabs(CloseTabMode mode);

signals:
     void allRunControlsFinished();

public slots:
    // ApplicationOutput specifics
    void projectRemoved();

    void appendMessage(ProjectExplorer::RunControl *rc, const QString &out,
                       ProjectExplorer::OutputFormat format);

private slots:
    void reRunRunControl();
    void stopRunControl();
    bool closeTab(int index);
    void tabChanged(int);
    void runControlStarted();
    void runControlFinished();

    void aboutToUnloadSession();

private:
    struct RunControlTab {
        explicit RunControlTab(RunControl *runControl = 0,
                               OutputWindow *window = 0);
        RunControl* runControl;
        OutputWindow *window;
        // Is the run control stopping asynchronously, close the tab once it finishes
        bool asyncClosing;
    };

    bool isRunning() const;
    bool closeTab(int index, CloseTabMode cm);
    bool optionallyPromptToStop(RunControl *runControl);

    int indexOf(const RunControl *) const;
    int indexOf(const QWidget *outputWindow) const;
    int currentIndex() const;
    RunControl *currentRunControl() const;
    int tabWidgetIndexOf(int runControlIndex) const;

    QWidget *m_mainWidget;
    QTabWidget *m_tabWidget;
    QList<RunControlTab> m_runControlTabs;
    QAction *m_stopAction;
    QToolButton *m_reRunButton;
    QToolButton *m_stopButton;
    QIcon m_runIcon;
};


class OutputWindow : public QPlainTextEdit
{
    Q_OBJECT

public:
    OutputWindow(QWidget *parent = 0);
    ~OutputWindow();

    OutputFormatter* formatter() const;
    void setFormatter(OutputFormatter *formatter);

    void appendMessage(const QString &out, OutputFormat format);
    /// appends a \p text using \p format without using formater
    void appendText(const QString &text, const QTextCharFormat &format, int maxLineCount);

    void grayOutOldContent();

    void showEvent(QShowEvent *);

    void clear();
    void handleOldOutput();

    void scrollToBottom();

protected:
    bool isScrollbarAtBottom() const;

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *e);
    virtual void keyPressEvent(QKeyEvent *ev);

private slots:
    void updateWordWrapMode();

private:
    void enableUndoRedo();
    QString doNewlineEnfocement(const QString &out);

    Core::BaseContext *m_outputWindowContext;
    OutputFormatter *m_formatter;

    bool m_enforceNewline;
    bool m_scrollToBottom;
    bool m_linksActive;
    bool m_mousePressed;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // OUTPUTWINDOW_H
