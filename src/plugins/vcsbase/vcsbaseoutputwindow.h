/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef VCSBASEOUTPUTWINDOW_H
#define VCSBASEOUTPUTWINDOW_H

#include "vcsbase_global.h"

#include  <coreplugin/ioutputpane.h>

namespace VCSBase {

struct VCSBaseOutputWindowPrivate;

/* Common OutputWindow for Version Control System command and other output.
 * Installed by the base plugin and accessible for the other plugins
 * via static instance()-accessor. Provides slots to append output with
 * special formatting.
 * It is possible to associate a repository with plain log text, enabling
 * an "Open" context menu action over relative file name tokens in the text
 * (absolute paths will also work). This can be used for "status" logs,
 * showing modified file names, allowing the user to open them. */

class VCSBASE_EXPORT VCSBaseOutputWindow : public Core::IOutputPane
{
    Q_OBJECT
    Q_PROPERTY(QString repository READ repository WRITE setRepository)
public:
    virtual ~VCSBaseOutputWindow();

    virtual QWidget *outputWidget(QWidget *parent);
    virtual QList<QWidget*> toolBarWidgets() const;
    virtual QString displayName() const;

    virtual int priorityInStatusBar() const;

    virtual void clearContents();
    virtual void visibilityChanged(bool visible);

    virtual void setFocus();
    virtual bool hasFocus();
    virtual bool canFocus();

    virtual bool canNavigate();
    virtual bool canNext();
    virtual bool canPrevious();
    virtual void goToNext();
    virtual void goToPrev();

    static VCSBaseOutputWindow *instance();

    QString repository() const;

    // Helper to consistently format log entries for commands as
    // 'Executing <dir>: <cmd> <args>'. Hides well-known password option
    // arguments.
    static QString msgExecutionLogEntry(const QString &workingDir,
                                        const QString &executable,
                                        const QStringList &arguments);

public slots:
    void setRepository(const QString &);
    void clearRepository();

    // Set the whole text.
    void setText(const QString &text);
    // Set text from QProcess' output data using the Locale's converter.
    void setData(const QByteArray &data);

    // Append text and pop up.
    void append(const QString &text);
    // Append data using the Locale's converter and pop up.
    void appendData(const QByteArray &data);

    // Silently append text, do not pop up.
    void appendSilently(const QString &text);
    // Silently append data using the Locale's converter, do not pop up.
    void appendDataSilently(const QByteArray &data);

    // Append red error text and pop up.
    void appendError(const QString &text);

    // Append dark-yellow warning text and pop up.
    void appendWarning(const QString &text);

    // Append a command, prepended by a log time stamp. "Executing: vcs -diff"
    // will result in "10:00 Executing: vcs -diff" in bold
    void appendCommand(const QString &text);
    // Append a standard-formatted entry for command execution
    // (see msgExecutionLogEntry).
    void appendCommand(const QString &workingDirectory,
                       const QString &binary,
                       const QStringList &args);

private:
    VCSBaseOutputWindow();

    VCSBaseOutputWindowPrivate *d;
};

} // namespace VCSBase

#endif // VCSBASEOUTPUTWINDOW_H
