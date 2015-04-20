/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

private:
    CompileOutputTextEdit *m_outputWindow;
    QHash<unsigned int, QPair<int, int>> m_taskPositions;
    ShowOutputTaskHandler * m_handler;
    QToolButton *m_cancelBuildButton;
    Utils::AnsiEscapeCodeHandler *m_escapeCodeHandler;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // COMPILEOUTPUTWINDOW_H
