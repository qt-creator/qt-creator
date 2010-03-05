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
    virtual QString name() const;

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

private:
    VCSBaseOutputWindow();

    VCSBaseOutputWindowPrivate *d;
};

} // namespace VCSBase

#endif // VCSBASEOUTPUTWINDOW_H
