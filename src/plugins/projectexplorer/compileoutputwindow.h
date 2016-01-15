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

#ifndef COMPILEOUTPUTWINDOW_H
#define COMPILEOUTPUTWINDOW_H

#include "buildstep.h"
#include <coreplugin/ioutputpane.h>

#include <QHash>
#include <QPair>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QTextCharFormat;
class QToolButton;
QT_END_NAMESPACE

namespace Utils { class AnsiEscapeCodeHandler; }

namespace ProjectExplorer {

class BuildManager;
class Task;

namespace Internal {

class ShowOutputTaskHandler;
class CompileOutputTextEdit;

class CompileOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    CompileOutputWindow(QAction *cancelBuildAction);
    ~CompileOutputWindow();

    QWidget *outputWidget(QWidget *);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const { return tr("Compile Output"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void appendText(const QString &text, BuildStep::OutputFormat format);
    bool canFocus() const;
    bool hasFocus() const;
    void setFocus();

    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    bool canNavigate() const;

    void registerPositionOf(const Task &task, int linkedOutputLines, int skipLines);
    bool knowsPositionOf(const Task &task);
    void showPositionOf(const Task &task);

    void flush();

private slots:
    void updateWordWrapMode();
    void updateZoomEnabled();

private:
    CompileOutputTextEdit *m_outputWindow;
    QHash<unsigned int, QPair<int, int>> m_taskPositions;
    ShowOutputTaskHandler * m_handler;
    QToolButton *m_cancelBuildButton;
    QToolButton *m_zoomInButton;
    QToolButton *m_zoomOutButton;
    Utils::AnsiEscapeCodeHandler *m_escapeCodeHandler;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // COMPILEOUTPUTWINDOW_H
