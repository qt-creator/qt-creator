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

#ifndef VCSOUTPUTWINDOW_H
#define VCSOUTPUTWINDOW_H

#include "vcsbase_global.h"

#include  <coreplugin/ioutputpane.h>

namespace Utils { class FileName; }
namespace VcsBase {

class VCSBASE_EXPORT VcsOutputWindow : public Core::IOutputPane
{
    Q_OBJECT
    Q_PROPERTY(QString repository READ repository WRITE setRepository)

public:
    ~VcsOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget *> toolBarWidgets() const;
    QString displayName() const;

    int priorityInStatusBar() const;

    void clearContents();
    void visibilityChanged(bool visible);

    void setFocus();
    bool hasFocus() const;
    bool canFocus() const;

    bool canNavigate() const;
    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();

    static VcsOutputWindow *instance();

    QString repository() const;

    // Helper to consistently format log entries for commands as
    // 'Executing <dir>: <cmd> <args>'. Hides well-known password option
    // arguments.
    static QString msgExecutionLogEntry(const QString &workingDir,
                                        const Utils::FileName &executable,
                                        const QStringList &arguments);

    enum MessageStyle {
        None,
        Error, // Red error text
        Warning, // Dark yellow warning text
        Command, // A bold command with timetamp "10:00 " + "Executing: vcs -diff"
        Message, // A blue message text (e.g. "command has finished successfully")
    };

public slots:
    static void setRepository(const QString &);
    static void clearRepository();

    // Set the whole text.
    static void setText(const QString &text);
    // Set text from QProcess' output data using the Locale's converter.
    static void setData(const QByteArray &data);

    // Append text with a certain style (none by default),
    // and maybe pop up (silent by default)
    static void append(const QString &text, enum MessageStyle style = None, bool silently = false);

    // Silently append text, do not pop up.
    static void appendSilently(const QString &text);

    // Append red error text and pop up.
    static void appendError(const QString &text);

    // Append dark-yellow warning text and pop up.
    static void appendWarning(const QString &text);

    // Append a command, prepended by a log time stamp. "Executing: vcs -diff"
    // will result in "10:00 Executing: vcs -diff" in bold
    // Filter passwords from URLs while doing this.
    static void appendShellCommandLine(const QString &text);

    // Append a standard-formatted entry for command execution
    // (see msgExecutionLogEntry).
    static void appendCommand(const QString &workingDirectory,
                       const Utils::FileName &binary,
                       const QStringList &args);

    // Append a blue message text and pop up.
    static void appendMessage(const QString &text);

private:
    VcsOutputWindow();
};

} // namespace VcsBase

#endif // VCSOUTPUTWINDOW_H
