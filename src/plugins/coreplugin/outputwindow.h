/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include "core_global.h"
#include "icontext.h"

#include <utils/outputformatter.h>

#include <QPlainTextEdit>

namespace Core {

class IContext;

class CORE_EXPORT OutputWindow : public QPlainTextEdit
{
    Q_OBJECT

public:
    OutputWindow(Core::Context context, QWidget *parent = 0);
    ~OutputWindow();

    Utils::OutputFormatter* formatter() const;
    void setFormatter(Utils::OutputFormatter *formatter);

    void appendMessage(const QString &out, Utils::OutputFormat format);
    /// appends a \p text using \p format without using formater
    void appendText(const QString &text, const QTextCharFormat &format = QTextCharFormat());

    void grayOutOldContent();
    void clear();

    void showEvent(QShowEvent *);

    void scrollToBottom();

    void setMaxLineCount(int count);
    int maxLineCount() const { return m_maxLineCount; }

public slots:
    void setWordWrapEnabled(bool wrap);

protected:
    bool isScrollbarAtBottom() const;

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
    virtual void resizeEvent(QResizeEvent *e);
    virtual void keyPressEvent(QKeyEvent *ev);

private:
    void enableUndoRedo();
    QString doNewlineEnfocement(const QString &out);

    Core::IContext *m_outputWindowContext;
    Utils::OutputFormatter *m_formatter;

    bool m_enforceNewline;
    bool m_scrollToBottom;
    bool m_linksActive;
    bool m_mousePressed;
    int m_maxLineCount;
};

} // namespace Core

#endif // OUTPUTWINDOW_H
